//*      _ _                      _     _            *
//*   __| | |__     __ _ _ __ ___| |__ (_)_   _____  *
//*  / _` | '_ \   / _` | '__/ __| '_ \| \ \ / / _ \ *
//* | (_| | |_) | | (_| | | | (__| | | | |\ V /  __/ *
//*  \__,_|_.__/   \__,_|_|  \___|_| |_|_| \_/ \___| *
//*                                                  *
//===- unittests/core/test_db_archive.cpp ---------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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

#include "pstore/core/db_archive.hpp"

// Stadard includes
#include <array>
#include <cstdint>

// 3rd party includes
#include <gmock/gmock.h>

// pstore public includes
#include "pstore/support/gsl.hpp"
#include "pstore/core/transaction.hpp"

// Local test includes
#include "empty_store.hpp"
#include "mock_mutex.hpp"

namespace {

    template <typename Transaction>
    pstore::typed_address<std::uint64_t> append_uint64 (Transaction & transaction,
                                                        std::uint64_t v) {
        auto addr = pstore::typed_address<std::uint64_t>::null ();
        std::shared_ptr<std::uint64_t> ptr;
        std::tie (ptr, addr) = transaction.template alloc_rw<std::uint64_t> ();
        *ptr = v;
        return addr;
    }

    class DbArchive : public EmptyStore {
    protected:
    };

} // end anonymous namespace

TEST_F (DbArchive, ReadASingleUint64) {
    pstore::database db{this->file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    std::uint64_t v1 = UINT64_C (0xF0F0F0F0F0F0F0F0);

    // Append v1 to the store (we don't need to have committed the transaction to be able to access
    // its contents).
    mock_mutex mutex;
    auto transaction = pstore::begin (db, std::unique_lock<mock_mutex>{mutex});
    pstore::typed_address<std::uint64_t> addr = append_uint64 (transaction, v1);

    // Now try reading it back again using a serializer.
    pstore::serialize::archive::database_reader archive (db, addr.to_address ());
    std::uint64_t const v2 = pstore::serialize::read<std::uint64_t> (archive);
    EXPECT_EQ (v1, v2);
}

TEST_F (DbArchive, ReadAUint64Span) {
    using ::testing::ContainerEq;

    pstore::database db{this->file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    std::array<std::uint64_t, 2> const original{{
        UINT64_C (0xF0F0F0F0F0F0F0F0),
        UINT64_C (0xFEEDFACECAFEBEEF),
    }};

    // Append 'original' to the store.
    mock_mutex mutex;
    auto transaction = pstore::begin (db, std::unique_lock<mock_mutex>{mutex});
    pstore::typed_address<std::uint64_t> addr = append_uint64 (transaction, original[0]);
    append_uint64 (transaction, original[1]);

    // Now the read the array back again.
    pstore::serialize::archive::database_reader archive (db, addr.to_address ());
    std::array<std::uint64_t, 2> actual;
    pstore::serialize::read (archive, ::pstore::gsl::span<std::uint64_t>{actual});

    EXPECT_THAT (original, ContainerEq (actual));
}

TEST_F (DbArchive, WriteASingleUint64) {
    pstore::database db{this->file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    std::uint64_t const original = UINT64_C (0xF0F0F0F0F0F0F0F0);

    // Write 'original' to the store using a serializer.
    mock_mutex mutex;
    auto transaction = pstore::begin (db, std::unique_lock<mock_mutex>{mutex});
    auto archive = pstore::serialize::archive::make_writer (transaction);
    auto where = pstore::typed_address<std::uint64_t>::make (db.size ());
    pstore::serialize::write (archive, original);

    // Now read that value back again using the raw pstore API and check that the round-trip was
    // successful.
    auto where2 = pstore::typed_address<std::uint64_t>::make (
        where.absolute () + pstore::calc_alignment (where.absolute (), alignof (std::uint64_t)));
    auto actual = db.getro (where2);
    EXPECT_EQ (original, *actual);
}

TEST_F (DbArchive, WriteAUint64Span) {
    pstore::database db{this->file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    std::array<std::uint64_t, 2> const original{{
        UINT64_C (0xF0F0F0F0F0F0F0F0),
        UINT64_C (0xFEEDFACECAFEBEEF),
    }};

    // Write the 'original' array span to the store using a serializer.
    mock_mutex mutex;
    auto transaction = pstore::begin (db, std::unique_lock<mock_mutex>{mutex});
    auto where = pstore::address::make (db.size ());
    pstore::serialize::write (pstore::serialize::archive::make_writer (transaction),
                              pstore::gsl::make_span (original));

    // Now read that value back again using the raw pstore API and check that the round-trip was
    // successful.
    auto where2 = pstore::typed_address<std::uint64_t>::make (
        where.absolute () + pstore::calc_alignment (where.absolute (), alignof (std::uint64_t)));
    std::shared_ptr<std::uint64_t const> actual = db.getro (where2, 2);
    EXPECT_EQ (original[0], (actual.get ())[0]);
    EXPECT_EQ (original[1], (actual.get ())[1]);
}


namespace {

    class DbArchiveWriteSpan : public EmptyStore {
    protected:
        class mock_transaction : public pstore::transaction<std::unique_lock<mock_mutex>> {
        public:
            using inherited = pstore::transaction<std::unique_lock<mock_mutex>>;
            mock_transaction (pstore::database & db, inherited::lock_type && lock)
                    : inherited (db, std::move (lock)) {}

            MOCK_METHOD2 (allocate, pstore::address (std::uint64_t /*size*/, unsigned /*align*/));
            pstore::address base_allocate (std::uint64_t size, unsigned align) {
                return inherited::allocate (size, align);
            }
        };
    };

} // end anonymous namespace

TEST_F (DbArchiveWriteSpan, WriteUint64Span) {
    using ::testing::_;
    using ::testing::Invoke;

    pstore::database db{this->file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    std::array<std::uint64_t, 2> original{{
        UINT64_C (0x0011223344556677),
        UINT64_C (0x8899AABBCCDDEEFF),
    }};
    auto const span = ::pstore::gsl::make_span (original);

    mock_mutex mutex;
    mock_transaction transaction (db, std::unique_lock<mock_mutex>{mutex});
    // All calls to transaction.alloc_rw are forwarded to the real implementation. We
    // expect there to be a single call to alloc_rw() which is suitable for the space
    // and alignment requirement of the entire span.
    auto invoke_base_allocate = Invoke (&transaction, &mock_transaction::base_allocate);
    EXPECT_CALL (transaction, allocate (_, _)).WillRepeatedly (invoke_base_allocate);
    EXPECT_CALL (transaction, allocate (sizeof (original), alignof (std::uint64_t)))
        .Times (1)
        .WillOnce (invoke_base_allocate);

    // Write the span.
    using namespace pstore::serialize;
    write (archive::make_writer (transaction), span);
}

namespace {

    class DbArchiveReadSpan : public EmptyStore {
    protected:
        class mock_database : public pstore::database {
        public:
            explicit mock_database (std::shared_ptr<pstore::file::in_memory> file)
                    : pstore::database (std::move (file)) {

                this->set_vacuum_mode (pstore::database::vacuum_mode::disabled);
            }

            MOCK_CONST_METHOD4 (get, std::shared_ptr<void const> (pstore::address, std::size_t,
                                                                  bool /*is_initialized*/,
                                                                  bool /*is_writable*/));

            auto base_get (pstore::address const & addr, std::size_t size, bool is_initialized,
                           bool is_writable) const -> std::shared_ptr<void const> {
                return pstore::database::get (addr, size, is_initialized, is_writable);
            }
        };
    };

} // end anonymous namespace

TEST_F (DbArchiveReadSpan, ReadUint64Span) {
    using ::testing::_;
    using ::testing::Invoke;

    mock_database db{this->file ()};

    // All calls to db.get() are forwarded to the real implementation.
    auto invoke_base_get = Invoke (&db, &mock_database::base_get);
    EXPECT_CALL (db, get (_, _, _, _)).WillRepeatedly (invoke_base_get);

    // Append 'original' to the store.
    std::array<std::uint64_t, 2> const original{{
        UINT64_C (0xF0F0F0F0F0F0F0F0),
        UINT64_C (0xFEEDFACECAFEBEEF),
    }};

    mock_mutex mutex;
    auto transaction = pstore::begin (db, std::unique_lock<mock_mutex>{mutex});
    pstore::typed_address<std::uint64_t> addr = append_uint64 (transaction, original[0]);
    append_uint64 (transaction, original[1]);

    // Now use the serializer to read a span of two uint64_ts. We expect the database.get()
    // method to be called exactly once for this operation.
    std::array<std::uint64_t, 2> actual;
    EXPECT_CALL (
        db, get (addr.to_address (), sizeof (actual), true /*initialized*/, false /*writable*/))
        .Times (1)
        .WillOnce (invoke_base_get);

    using namespace pstore::serialize;
    read (archive::database_reader (db, addr.to_address ()),
          pstore::gsl::span<std::uint64_t>{actual});
}
