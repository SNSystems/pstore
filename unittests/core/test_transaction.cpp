//*  _                                  _   _              *
//* | |_ _ __ __ _ _ __  ___  __ _  ___| |_(_) ___  _ __   *
//* | __| '__/ _` | '_ \/ __|/ _` |/ __| __| |/ _ \| '_ \  *
//* | |_| | | (_| | | | \__ \ (_| | (__| |_| | (_) | | | | *
//*  \__|_|  \__,_|_| |_|___/\__,_|\___|\__|_|\___/|_| |_| *
//*                                                        *
//===- unittests/pstore/test_transaction.cpp ------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
//===----------------------------------------------------------------------===//
#include "pstore/core/transaction.hpp"

#include <mutex>
#include <numeric>

#include "gmock/gmock.h"

#include "empty_store.hpp"
#include "mock_mutex.hpp"

namespace {

    class mock_database : public pstore::database {
    public:
        mock_database (std::shared_ptr<pstore::file::in_memory> const & file)
                : pstore::database (file) {

            this->set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

        MOCK_CONST_METHOD4 (get, std::shared_ptr<void const> (pstore::address, std::size_t,
                                                              bool /*is_initialized*/,
                                                              bool /*is_writable*/));
        MOCK_METHOD2 (allocate, pstore::address (std::uint64_t /*size*/, unsigned /*align*/));

        // The next two methods simply allow the mocks to call through to the base class's
        // implementation of allocate() and get().
        pstore::address base_allocate (std::uint64_t bytes, unsigned align) {
            return pstore::database::allocate (bytes, align);
        }
        auto base_get (pstore::address const & addr, std::size_t size, bool is_initialized,
                       bool is_writable) const -> std::shared_ptr<void const> {
            return pstore::database::get (addr, size, is_initialized, is_writable);
        }
    };

    class Transaction : public EmptyStore {
    protected:
        pstore::header const * get_header () {
            auto h = reinterpret_cast<pstore::header const *> (buffer_.get ());
            return h;
        }

        void SetUp () override {
            EmptyStore::SetUp ();
            db_.reset (new mock_database{file_});
            db_->set_vacuum_mode (pstore::database::vacuum_mode::disabled);
            using ::testing::_;
            using ::testing::Invoke;

            // Pass the mocked calls through to their original implementations.
            // I'm simply using the mocking framework to observe that the
            // correct calls are made. The precise division of labor between
            // the database and transaction classes is enforced or determined here.
            auto invoke_allocate = Invoke (db_.get (), &mock_database::base_allocate);
            auto invoke_get = Invoke (db_.get (), &mock_database::base_get);

            EXPECT_CALL (*db_, get (_, _, _, _)).WillRepeatedly (invoke_get);
            EXPECT_CALL (*db_, allocate (_, _)).WillRepeatedly (invoke_allocate);
        }

        void TearDown () override {
            db_.reset ();
            EmptyStore::TearDown ();
        }

        std::unique_ptr<mock_database> db_;
    };
} // namespace

TEST_F (Transaction, CommitEmptyDoesNothing) {

    pstore::database db{file_};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    // A quick check of the initial state.
    auto header = this->get_header ();
    ASSERT_EQ (sizeof (pstore::header), header->footer_pos.load ().absolute ());

    {
        mock_mutex mutex;
        typedef std::unique_lock<mock_mutex> guard_type;
        auto transaction = pstore::begin (db, guard_type{mutex});
        transaction.commit ();
    }

    EXPECT_EQ (sizeof (pstore::header), header->footer_pos.load ().absolute ());
}

TEST_F (Transaction, CommitInt) {

    pstore::database db{file_};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    auto header = this->get_header ();
    std::uint64_t const r0footer_offset = header->footer_pos.load ().absolute ();

    int const data_value = 32749;

    // Scope for the single transaction that we'll commit for the test.
    {
        mock_mutex mutex;
        auto transaction = pstore::begin (db, std::unique_lock<mock_mutex>{mutex});
        {
            // Write an integer to the store.
            // If rw is a spanning pointer, it will only be saved to the store when it is deleted.
            // TODO: write a large vector (>4K) so the 'protect' function, which is called by the
            // transaction commit function, has an effect.
            std::pair<std::shared_ptr<int>, pstore::typed_address<int>> rw =
                transaction.alloc_rw<int> ();
            ASSERT_EQ (0U, rw.second.absolute () % alignof (int))
                << "The address must be suitably aligned for int";
            *(rw.first) = data_value;
        }

        transaction.commit ();
    }

    std::uint64_t new_header_offset = sizeof (pstore::header);
    new_header_offset += sizeof (pstore::trailer);
    new_header_offset += pstore::calc_alignment (new_header_offset, alignof (int));
    std::uint64_t const r1contents_offset = new_header_offset;
    new_header_offset += sizeof (int);
    new_header_offset += pstore::calc_alignment (new_header_offset, alignof (pstore::trailer));

    std::uint64_t const g1footer_offset = header->footer_pos.load ().absolute ();
    ASSERT_EQ (new_header_offset, g1footer_offset)
        << "Expected offset of r1 footer to be " << new_header_offset;


    // Header checks.
    EXPECT_THAT (pstore::header::file_signature1, ::testing::ContainerEq (header->a.signature1))
        << "File header was missing";
    EXPECT_EQ (g1footer_offset, header->footer_pos.load ().absolute ());

    // Check the two footers.
    {
        auto r0footer =
            reinterpret_cast<pstore::trailer const *> (buffer_.get () + r0footer_offset);
        EXPECT_THAT (pstore::trailer::default_signature1,
                     ::testing::ContainerEq (r0footer->a.signature1))
            << "Did not find the r0 footer signature1";
        EXPECT_EQ (0U, r0footer->a.generation) << "r0 footer generation number must be 0";
        EXPECT_EQ (0U, r0footer->a.size) << "expected the r0 footer size value to be 0";
        EXPECT_EQ (pstore::typed_address<pstore::trailer>::null (), r0footer->a.prev_generation)
            << "The r0 footer should not point to a previous generation";
        EXPECT_THAT (pstore::trailer::default_signature2,
                     ::testing::ContainerEq (r0footer->signature2))
            << "Did not find r0 footer signature2";

        auto r1footer =
            reinterpret_cast<pstore::trailer const *> (buffer_.get () + g1footer_offset);
        EXPECT_THAT (pstore::trailer::default_signature1,
                     ::testing::ContainerEq (r1footer->a.signature1))
            << "Did not find the r1 footer signature1";
        EXPECT_EQ (1U, r1footer->a.generation) << "r1 footer generation number must be 1";
        EXPECT_GE (r1footer->a.size, sizeof (int)) << "r1 footer size must be at least sizeof (int";
        EXPECT_EQ (pstore::typed_address<pstore::trailer>::make (sizeof (pstore::header)),
                   r1footer->a.prev_generation)
            << "r1 previous pointer must point to r0 footer";
        EXPECT_THAT (pstore::trailer::default_signature2,
                     ::testing::ContainerEq (r1footer->signature2))
            << "Did not find r1 footer signature2";

        EXPECT_GE (r1footer->a.time, r0footer->a.time)
            << "r1 time must not be earlier than r0 time";
    }

    // Finally check the r1 contents
    {
        auto r1data = reinterpret_cast<int const *> (buffer_.get () + r1contents_offset);
        EXPECT_EQ (data_value, *r1data);
    }
}

TEST_F (Transaction, RollbackAfterAppendingInt) {

    pstore::database db{file_};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    // A quick check of the initial state.
    auto header = this->get_header ();
    ASSERT_EQ (sizeof (pstore::header), header->footer_pos.load ().absolute ());

    {
        mock_mutex mutex;
        typedef std::unique_lock<mock_mutex> guard_type;
        auto transaction = pstore::begin (db, guard_type{mutex});

        // Write an integer to the store.
        *(transaction.alloc_rw<int> ().first) = 42;

        // Abandon the transaction.
        transaction.rollback ();
    }

    // Header checks
    EXPECT_THAT (pstore::header::file_signature1, ::testing::ContainerEq (header->a.signature1))
        << "File header was missing";
    EXPECT_EQ (sizeof (pstore::header), header->footer_pos.load ().absolute ())
        << "Expected the file header footer_pos to point to r0 header";

    {
        auto r0footer =
            reinterpret_cast<pstore::trailer const *> (buffer_.get () + sizeof (pstore::header));
        EXPECT_THAT (pstore::trailer::default_signature1,
                     ::testing::ContainerEq (r0footer->a.signature1))
            << "Did not find r0 footer signature1";
        EXPECT_EQ (0U, r0footer->a.generation) << "r0 footer generation number must be 0";
        EXPECT_EQ (0U, r0footer->a.size);
        EXPECT_EQ (pstore::typed_address<pstore::trailer>::null (), r0footer->a.prev_generation);
        EXPECT_THAT (pstore::trailer::default_signature2,
                     ::testing::ContainerEq (r0footer->signature2))
            << "Did not find r0 footer signature2";
    }
}

TEST_F (Transaction, CommitAfterAppending4Mb) {

    pstore::database db{file_};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
    {
        mock_mutex mutex;
        auto transaction = pstore::begin (db, std::unique_lock<mock_mutex>{mutex});

        std::size_t const elements = (4U * 1024U * 1024U) / sizeof (int);
        transaction.allocate (elements * sizeof (int), 1 /*align*/);
        transaction.commit ();
    }

    // Check the two footers.
    {
        pstore::header const * const header = this->get_header ();
        pstore::typed_address<pstore::trailer> const r1_footer_offset = header->footer_pos;

        auto r1_footer = reinterpret_cast<pstore::trailer const *> (buffer_.get () +
                                                                    r1_footer_offset.absolute ());
        EXPECT_THAT (pstore::trailer::default_signature1,
                     ::testing::ContainerEq (r1_footer->a.signature1))
            << "Did not find r1 footer signature1";
        EXPECT_EQ (1U, r1_footer->a.generation) << "r1 footer generation number must be 1";
        EXPECT_EQ (4194304U, r1_footer->a.size);
        // EXPECT_EQ (pstore::address (sizeof (pstore::header)), r1footer->prev_generation) << "r1
        // previous pointer must point to r0 footer";
        EXPECT_THAT (pstore::trailer::default_signature2,
                     ::testing::ContainerEq (r1_footer->signature2))
            << "Did not find r1 footer signature2";

        pstore::typed_address<pstore::trailer> const r0_footer_offset =
            r1_footer->a.prev_generation;

        auto r0_footer = reinterpret_cast<pstore::trailer const *> (buffer_.get () +
                                                                    r0_footer_offset.absolute ());
        EXPECT_THAT (pstore::trailer::default_signature1,
                     ::testing::ContainerEq (r0_footer->a.signature1))
            << "Did not find r0 footer signature1";
        EXPECT_EQ (0U, r0_footer->a.generation) << "r0 footer generation number must be 0";
        EXPECT_EQ (0U, r0_footer->a.size) << "expected the r0 footer size value to be 0";
        EXPECT_EQ (pstore::typed_address<pstore::trailer>::null (), r0_footer->a.prev_generation)
            << "The r0 footer should not point to a previous generation";
        EXPECT_THAT (pstore::trailer::default_signature2,
                     ::testing::ContainerEq (r0_footer->signature2))
            << "Did not find r0 footer signature2";

        EXPECT_GE (r1_footer->a.time, r0_footer->a.time)
            << "r1 time must not be earlier than r0 time";
    }
}

TEST_F (Transaction, CommitAfterAppendingAndWriting4Mb) {

    static std::size_t initial_elements = pstore::address::segment_size -
                                          (sizeof (pstore::header) + sizeof (pstore::trailer)) -
                                          16 * sizeof (int);
    initial_elements /= sizeof (int);
    static std::size_t const elements = 32;

    pstore::database db{file_};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    auto addr = pstore::typed_address<int>::null ();
    {
        mock_mutex mutex;
        auto transaction = pstore::begin (db, std::unique_lock<mock_mutex>{mutex});
        {
            transaction.allocate (initial_elements * sizeof (int), alignof (int));
            addr = pstore::typed_address<int>::make (
                transaction.allocate (elements * sizeof (int), alignof (int)));
            std::shared_ptr<int> ptr = transaction.getrw (addr, elements);
            std::iota (ptr.get (), ptr.get () + elements, 0);
        }
        transaction.commit ();
    }
    {
        std::shared_ptr<int const> v = db.getro (addr, elements);
        std::vector<int> expected (elements);
        std::iota (std::begin (expected), std::end (expected), 0);
        std::vector<int> actual (v.get (), v.get () + elements);

        EXPECT_THAT (actual, ::testing::ContainerEq (expected));
    }
}


namespace {
    using mock_lock = std::unique_lock<mock_mutex>;

    void append_int (pstore::transaction<mock_lock> & transaction, int v) {
        *(transaction.alloc_rw<int> ().first) = v;
    }
} // namespace

TEST_F (Transaction, CommitTwoSeparateTransactions) {
    // Append to individual transactions, each containing a single int.
    {
        pstore::database db{file_};
        db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        mock_mutex mutex;
        {
            auto t1 = pstore::begin (db, mock_lock (mutex));
            append_int (t1, 1);
            t1.commit ();
        }
        {
            auto t2 = pstore::begin (db, mock_lock (mutex));
            append_int (t2, 2);
            t2.commit ();
        }
    }

    std::size_t footer2 = sizeof (pstore::header);
    footer2 += pstore::calc_alignment (footer2, alignof (pstore::trailer));
    footer2 += sizeof (pstore::trailer);

    footer2 += pstore::calc_alignment (footer2, alignof (int));
    footer2 += sizeof (int);
    footer2 += pstore::calc_alignment (footer2, alignof (pstore::trailer));
    footer2 += sizeof (pstore::trailer);

    footer2 += pstore::calc_alignment (footer2, alignof (int));
    footer2 += sizeof (int);
    footer2 += pstore::calc_alignment (footer2, alignof (pstore::trailer));

    pstore::header const * const header = this->get_header ();
    EXPECT_EQ (pstore::typed_address<pstore::trailer>::make (footer2), header->footer_pos.load ());
}

TEST_F (Transaction, GetRwInt) {

    // Use the alloc_rw<> convenience method to return a pointer to an int that has been freshly
    // allocated within the transaction.

    // First setup the mock expectations.
    {
        using ::testing::Expectation;
        using ::testing::Ge;
        using ::testing::Invoke;

        Expectation allocate_int =
            EXPECT_CALL (*db_.get (), allocate (sizeof (int), alignof (int)))
                .WillOnce (Invoke (db_.get (), &mock_database::base_allocate));

        // A call to get(). First argument (address) must lie beyond the initial transaction
        // and must request a writable int.
        EXPECT_CALL (*db_.get (), get (Ge (pstore::address::make (sizeof (pstore::header) +
                                                                  sizeof (pstore::trailer))),
                                       sizeof (int), false, true))
            .After (allocate_int)
            .WillOnce (Invoke (db_.get (), &mock_database::base_get));
    }
    // Now the real body of the test
    {
        mock_mutex mutex;
        auto transaction = pstore::begin (*db_, std::unique_lock<mock_mutex>{mutex});
        transaction.alloc_rw<int> ();
        transaction.commit ();
    }
}

TEST_F (Transaction, GetRoInt) {

    // Use the getro<> method to return a address to the first int in the store.

    // First setup the mock expectations.
    EXPECT_CALL (*db_, get (pstore::address::null (), sizeof (int), true, false))
        .WillOnce (::testing::Invoke (db_.get (), &mock_database::base_get));

    // Now the real body of the test
    {
        mock_mutex mutex;
        auto transaction = pstore::begin (*db_, std::unique_lock<mock_mutex>{mutex});
        db_->getro (pstore::typed_address<int>::null (), 1);
        transaction.commit ();
    }
}

TEST_F (Transaction, GetRwUInt64) {
    std::uint64_t const expected = 1ULL << 40;
    pstore::extent<std::uint64_t> extent;
    {
        mock_mutex mutex;
        auto transaction = pstore::begin (*db_, std::unique_lock<mock_mutex>{mutex});
        {
            // Allocate the storage
            pstore::address const addr =
                transaction.allocate (sizeof (std::uint64_t), alignof (std::uint64_t));
            extent =
                make_extent (pstore::typed_address<std::uint64_t> (addr), sizeof (std::uint64_t));
            std::shared_ptr<std::uint64_t> ptr = transaction.getrw (extent);

            // Save the data to the store
            *(ptr) = expected;
        }
        transaction.commit ();
    }
    EXPECT_EQ (expected, *db_->getro (extent));
}

// eof: unittests/pstore/test_transaction.cpp
