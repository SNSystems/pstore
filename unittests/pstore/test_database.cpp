//*      _       _        _                     *
//*   __| | __ _| |_ __ _| |__   __ _ ___  ___  *
//*  / _` |/ _` | __/ _` | '_ \ / _` / __|/ _ \ *
//* | (_| | (_| | || (_| | |_) | (_| \__ \  __/ *
//*  \__,_|\__,_|\__\__,_|_.__/ \__,_|___/\___| *
//*                                             *
//===- unittests/pstore/test_database.cpp ---------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
/// \file test_database.cpp

#include "pstore/database.hpp"

#include <limits>
#include <memory>
#include <mutex>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "pstore_support/portab.hpp"
#include "check_for_error.hpp"
#include "empty_store.hpp"

TEST_F (EmptyStore, CheckInitialState) {
    pstore::database db{file_};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    {
        auto header = reinterpret_cast<pstore::header const *> (buffer_.get ());
        EXPECT_THAT (pstore::header::file_signature1,
                     ::testing::ContainerEq (header->a.signature1));
        EXPECT_EQ (pstore::header::file_signature2, header->a.signature2);

        auto const expected = std::array<std::uint16_t, 2>{
            {pstore::header::major_version, pstore::header::minor_version}};
        EXPECT_THAT (expected, ::testing::ContainerEq (header->a.version));
        EXPECT_EQ (sizeof (pstore::header), header->a.header_size);
        // std::uint8_t sync_name[sync_name_length];
        EXPECT_EQ (sizeof (pstore::header), header->footer_pos.load ().absolute ());
    }
    {
        std::uint64_t const offset = sizeof (pstore::header);
        auto footer = reinterpret_cast<pstore::trailer const *> (buffer_.get () + offset);

        EXPECT_THAT (pstore::trailer::default_signature1,
                     ::testing::ContainerEq (footer->a.signature1));
        EXPECT_EQ (0U, footer->a.generation);
        EXPECT_EQ (0U, footer->a.size);
        EXPECT_EQ (pstore::address::null (), footer->a.prev_generation);
        // std::atomic<std::uint64_t> time{0};
        // index_records_array index_records;
        EXPECT_THAT (pstore::trailer::default_signature2,
                     ::testing::ContainerEq (footer->signature2));
    }
}


namespace {
    class OpenCorruptStore : public EmptyStore {
    protected:
        pstore::header * get_header ();
        void check_database_open (pstore::error_code err) const;
    };

    pstore::header * OpenCorruptStore::get_header () {
        return reinterpret_cast<pstore::header *> (buffer_.get ());
    }

    void OpenCorruptStore::check_database_open (pstore::error_code err) const {
        auto fn = [this] () {
            pstore::database{file_};
        };
        check_for_error (fn, err);
    }
} // end anonymous namespace

TEST_F (OpenCorruptStore, HeaderBadSignature1) {
    // Modify the signature1 field.
    auto & s1 = this->get_header ()->a.signature1;
    s1[0] = !s1[0];
    this->check_database_open (pstore::error_code::header_corrupt);
}

TEST_F (OpenCorruptStore, HeaderBadSignature2) {
    // Modify the signature2 field.
    auto & s2 = this->get_header ()->a.signature2;
    s2 = !s2;
    this->check_database_open (pstore::error_code::header_corrupt);
}

TEST_F (OpenCorruptStore, HeaderBadSize) {
    auto h = this->get_header ();
    h->a.header_size = 0;
    this->check_database_open (pstore::error_code::header_version_mismatch);
}

TEST_F (OpenCorruptStore, HeaderBadMajorVersion) {
    auto h = this->get_header ();
    h->a.version[0] = std::numeric_limits<std::uint16_t>::max ();
    this->check_database_open (pstore::error_code::header_version_mismatch);
}

TEST_F (OpenCorruptStore, HeaderBadMinorVersion) {
    auto h = this->get_header ();
    h->a.version[1] = std::numeric_limits<std::uint16_t>::max ();
    this->check_database_open (pstore::error_code::header_version_mismatch);
}

TEST_F (OpenCorruptStore, HeaderUUID) {
    // This test is only valid if CRC checking is enabled.
    if (pstore::database::crc_checks_enabled ()) {
        auto h = this->get_header ();
        pstore::uuid & uuid = h->a.uuid;
        pstore::uuid::container_type old = uuid.array ();
        old[0] = !old[0];

        // Rebuild the UUID in place.
        h->a.uuid.~uuid ();
        new (&uuid) pstore::uuid (old);

        // An inconsistency in the sync_name field is caught by the CRC value no longer matching.
        // If CRC checks are disabled, then it's caught by the sync-name alphabet check.
        this->check_database_open (pstore::error_code::header_corrupt);
    }
}

TEST_F (OpenCorruptStore, HeaderFooterTooSmall) {
    // Footer_pos must be at least the size for of the file header...
    auto h = this->get_header ();
    h->footer_pos = pstore::address::null ();
    this->check_database_open (pstore::error_code::header_corrupt);
}

TEST_F (OpenCorruptStore, HeaderFooterTooLarge) {
    auto too_large =
        pstore::address::make ((UINT64_C (1) << (pstore::address::offset_number_bits +
                                                 pstore::address::segment_number_bits)) -
                               1U);
    this->get_header ()->footer_pos = too_large;
    this->check_database_open (pstore::error_code::header_corrupt);
}



TEST_F (EmptyStore, SegmentBase) {
    // Checks that the first segment address is equal to the address of our file backing store
    // buffer, and that all of the other segment pointers are null.

    pstore::database db{file_};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    static_assert (pstore::address::segment_size == pstore::storage::min_region_size,
                   "expected min_region_size == segment_size");

    auto ptr = buffer_.get ();

    ASSERT_LE (std::numeric_limits<pstore::address::segment_type>::max (), pstore::sat_elements);

    EXPECT_EQ (std::shared_ptr<void> (buffer_, ptr), db.storage ().segment_base (0));
    for (unsigned segment = 1; segment < pstore::sat_elements; ++segment) {
        auto const si = static_cast<pstore::address::segment_type> (segment);
        EXPECT_EQ (nullptr, db.storage ().segment_base (si));
    }

    // Now the same tests, but for the const method.
    auto const * dbp = &db;

    EXPECT_EQ (std::shared_ptr<void> (buffer_, ptr), dbp->storage ().segment_base (0));
    for (unsigned segment = 1; segment < pstore::sat_elements; ++segment) {
        auto const si = static_cast<pstore::address::segment_type> (segment);
        EXPECT_EQ (nullptr, dbp->storage ().segment_base (si));
    }
}

TEST_F (EmptyStore, GetEndPastLogicalEOF) {
    pstore::database db{file_};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    auto const addr = pstore::address::null ();
    std::size_t size = db.size () + 1;
    check_for_error ([&] () {
        db.getro (addr, size);
    }, pstore::error_code::bad_address);
}

TEST_F (EmptyStore, GetStartPastLogicalEOF) {
    pstore::database db{file_};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    auto const addr = pstore::address::make (db.size () + 1);
    std::size_t size = 1;
    check_for_error ([&] () {
        db.getro (addr, size);
    }, pstore::error_code::bad_address);
}

TEST_F (EmptyStore, GetLocationOverflows) {
    pstore::database db{file_};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    auto const addr = std::numeric_limits<pstore::address>::max ();
    STATIC_ASSERT (std::numeric_limits<std::size_t>::max () >=
                   std::numeric_limits<std::uint64_t>::max ());
    std::size_t const size = std::numeric_limits<std::uint64_t>::max () - addr.absolute () + 1U;
    ASSERT_TRUE (addr + size < addr); // This addition is attended to overflow.
    check_for_error ([&] () {
        db.getro (addr, size);
    }, pstore::error_code::bad_address);
}



namespace {
    class mock_mutex {
    public:
        void lock () {}
        void unlock () {}
    };
}

TEST_F (EmptyStore, Allocate16Bytes) {
    pstore::database db{file_};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    mock_mutex mutex;
    std::lock_guard<mock_mutex> lock (mutex);

    // Initial allocation.
    constexpr auto size = 16U;
    constexpr auto align = 1U;
    pstore::address addr = db.allocate (lock, size, align);
    EXPECT_EQ (sizeof (pstore::header) + sizeof (pstore::trailer), addr.absolute ());

    // Subsequent allocation.
    pstore::address addr2 = db.allocate (lock, size, align);
    EXPECT_EQ (addr.absolute () + 16, addr2.absolute ());
}

TEST_F (EmptyStore, Allocate16BytesAligned1024) {
    pstore::database db{file_};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    mock_mutex mutex;
    std::lock_guard<mock_mutex> lock (mutex);

    constexpr unsigned size = 16;
    constexpr unsigned align = 1024;
    STATIC_ASSERT (align > sizeof (pstore::header) + sizeof (pstore::trailer));

    pstore::address addr = db.allocate (lock, size, align);
    EXPECT_EQ (0U, addr.absolute () % align);

    pstore::address addr2 = db.allocate (lock, size, align);
    EXPECT_EQ (addr.absolute () + align, addr2.absolute ());
}


namespace {
    class fixed_page_size final : public pstore::system_page_size_interface {
    public:
        MOCK_CONST_METHOD0 (get, unsigned());
    };


    class mock_mapper : public pstore::in_memory_mapper {
    public:
        mock_mapper (pstore::file::in_memory & file, bool write_enabled, std::uint64_t offset,
                     std::uint64_t length)
                : pstore::in_memory_mapper (file, write_enabled, offset, length) {}

        MOCK_METHOD2 (read_only, void(void * addr, std::size_t len));
    };


    class mock_region_factory final : public pstore::region::factory {
    public:
        mock_region_factory (std::shared_ptr<pstore::file::in_memory> file, std::uint64_t full_size,
                             std::uint64_t min_size)
                : pstore::region::factory (full_size, min_size)
                , file_ (std::move (file)) {

            assert (full_size >= min_size);
            assert (full_size % pstore::address::segment_size == 0);
            assert (min_size % pstore::address::segment_size == 0);
        }

        auto init () -> std::vector<pstore::region::memory_mapper_ptr> override {
            return this->create<pstore::file::in_memory, mock_mapper> (file_);
        }

        void add (std::vector<pstore::region::memory_mapper_ptr> * const regions,
                  std::uint64_t original_size, std::uint64_t new_size) override {

            this->append<pstore::file::in_memory, mock_mapper> (file_, regions, original_size,
                                                                new_size);
        }

        std::shared_ptr<pstore::file::file_base> file () override {
            return file_;
        }

    private:
        std::shared_ptr<pstore::file::in_memory> file_;
    };
}


namespace {
    template <typename T, typename U>
    std::shared_ptr <T> cast (std::shared_ptr <U> p) {
#ifdef PSTORE_CPP_RTTI
        return std::dynamic_pointer_cast <T> (p);
#else
        return std::static_pointer_cast <T> (p);
#endif
    }
} // end anonymous namespace


TEST_F (EmptyStore, ProtectAllOfOneRegion) {
    using ::testing::Return;
    using ::testing::Ref;
    using ::testing::_;

    auto const fixed_page_size_bytes = 4096U;
    auto page_size = std::make_unique<fixed_page_size> ();
    EXPECT_CALL (*page_size, get ()).WillRepeatedly (Return (fixed_page_size_bytes));

    // Create the data store instance. It will use 4K pages mapped using mock_mapper instances.
    pstore::database db{file_, std::move (page_size), std::make_unique<mock_region_factory> (
                                                          file_, pstore::storage::min_region_size,
                                                          pstore::storage::min_region_size)};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);


    pstore::storage::region_container const & regions = db.storage ().regions ();
    EXPECT_EQ (1U, regions.size ()) << "Expected the store to use 1 region";

    // I expect db.protect() to call mock_mapper::read_only() once with
    // - The first parameter should equal one page into the memory block (the first page of the
    //  data store must remain writable.
    // - The second should be the size of the file minus the size of that missing first page.
    auto r0 = cast<mock_mapper> (regions.at (0));
    void * addr = reinterpret_cast<std::uint8_t *> (file_->data ().get ()) + fixed_page_size_bytes;

    // If "POSIX small file" mode is enabled, the file will be smaller than a VM page (4K), so
    // read_only() won't be called at all.
    if (pstore::database::small_files_enabled ()) {
        EXPECT_CALL (*r0.get (), read_only (_, _)).Times (0);
    } else {
        EXPECT_CALL (*r0.get (), read_only (addr, file_->size () - fixed_page_size_bytes))
            .Times (1);
    }

    db.protect (pstore::address::null (), pstore::address::make (file_->size ()));
}


// FIXME: this include is in the wrong place. Is the test in the wrong place?
#include "pstore/transaction.hpp"

TEST_F (EmptyStore, ProtectAllOfTwoRegions) {
    using ::testing::Return;
    using ::testing::Ref;

    auto const fixed_page_size_bytes = 4096U;
    auto page_size = std::make_unique<fixed_page_size> ();
    EXPECT_CALL (*page_size, get ()).WillRepeatedly (Return (fixed_page_size_bytes));

    // Create the data store instance. It will use 4K pages mapped using mock_mapper instances.
    pstore::database db{file_, std::move (page_size),
                        std::make_unique<mock_region_factory> (file_, pstore::address::segment_size,
                                                               pstore::address::segment_size)};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);


    mock_mutex mutex;
    auto transaction = pstore::begin (db, std::unique_lock<mock_mutex>{mutex});
    transaction.allocate (pstore::address::segment_size + 4096U /*size*/, 1 /*align*/);

    pstore::storage::region_container const & regions = db.storage ().regions ();
    EXPECT_EQ (2U, regions.size ()) << "Expected the store to use two regions";
    auto r0 = cast<mock_mapper> (regions.at (0));
    auto r1 = cast<mock_mapper> (regions.at (1));

    // I expect transaction commit to call read_only() on the first memory-mapped region once with:
    // - The first parameter equal to one page into the memory block (the first page of the
    //  data store must remain writable.
    // - The second should be the size of the region minus the size of that missing first page.
    std::uint64_t r0_protect_size = r0->size () - fixed_page_size_bytes;
    EXPECT_CALL (*r0.get (), read_only (reinterpret_cast<std::uint8_t *> (file_->data ().get ()) +
                                            fixed_page_size_bytes,
                                        r0_protect_size))
        .Times (1);

    EXPECT_CALL (*r1.get (), read_only (reinterpret_cast<std::uint8_t *> (file_->data ().get ()) +
                                            pstore::address::segment_size,
                                        4096U))
        .Times (1);

    transaction.commit ();
}

// eof: unittests/pstore/test_database.cpp
