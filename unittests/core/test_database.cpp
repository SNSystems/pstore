//*      _       _        _                     *
//*   __| | __ _| |_ __ _| |__   __ _ ___  ___  *
//*  / _` |/ _` | __/ _` | '_ \ / _` / __|/ _ \ *
//* | (_| | (_| | || (_| | |_) | (_| \__ \  __/ *
//*  \__,_|\__,_|\__\__,_|_.__/ \__,_|___/\___| *
//*                                             *
//===- unittests/core/test_database.cpp -----------------------------------===//
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
/// \file test_database.cpp

#include "pstore/core/database.hpp"

#include <limits>
#include <memory>
#include <mutex>
#include <vector>

#include "gmock/gmock.h"

#include "pstore/support/portab.hpp"

#include "check_for_error.hpp"
#include "empty_store.hpp"
#include "mock_mutex.hpp"

namespace {
    class Database : public EmptyStore {};
} // namespace

TEST_F (Database, CheckInitialState) {
    pstore::database db{this->file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    {
        auto header = reinterpret_cast<pstore::header const *> (this->buffer ().get ());
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
        auto footer = reinterpret_cast<pstore::trailer const *> (this->buffer ().get () + offset);

        EXPECT_THAT (pstore::trailer::default_signature1,
                     ::testing::ContainerEq (footer->a.signature1));
        EXPECT_EQ (0U, footer->a.generation);
        EXPECT_EQ (0U, footer->a.size);
        EXPECT_EQ (pstore::typed_address<pstore::trailer>::null (), footer->a.prev_generation);
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
        return reinterpret_cast<pstore::header *> (this->buffer ().get ());
    }

    void OpenCorruptStore::check_database_open (pstore::error_code err) const {
        check_for_error ([this]() { pstore::database{this->file ()}; }, err);
    }
} // end anonymous namespace

TEST_F (OpenCorruptStore, HeaderBadSignature1) {
#if PSTORE_SIGNATURE_CHECKS_ENABLED
    // Modify the signature1 field.
    auto * const h = this->get_header ();
    auto & s1 = h->a.signature1;
    s1[0] = !s1[0];
    h->crc = h->get_crc ();
    this->check_database_open (pstore::error_code::header_corrupt);
#endif // PSTORE_SIGNATURE_CHECKS_ENABLED
}

TEST_F (OpenCorruptStore, HeaderBadSignature2) {
#if PSTORE_SIGNATURE_CHECKS_ENABLED
    // Modify the signature2 field.
    auto * const h = this->get_header ();
    auto & s2 = h->a.signature2;
    s2 = !s2;
    h->crc = h->get_crc ();
    this->check_database_open (pstore::error_code::header_corrupt);
#endif // PSTORE_SIGNATURE_CHECKS_ENABLED
}

TEST_F (OpenCorruptStore, HeaderBadSize) {
    auto * const h = this->get_header ();
    h->a.header_size = 0;
    h->crc = h->get_crc ();
    this->check_database_open (pstore::error_code::header_version_mismatch);
}

TEST_F (OpenCorruptStore, HeaderBadMajorVersion) {
    auto * const h = this->get_header ();
    h->a.version[0] = std::numeric_limits<std::uint16_t>::max ();
    h->crc = h->get_crc ();
    this->check_database_open (pstore::error_code::header_version_mismatch);
}

TEST_F (OpenCorruptStore, HeaderBadMinorVersion) {
    auto * const h = this->get_header ();
    h->a.version[1] = std::numeric_limits<std::uint16_t>::max ();
    h->crc = h->get_crc ();
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
    h->footer_pos = pstore::typed_address<pstore::trailer>::null ();
    this->check_database_open (pstore::error_code::header_corrupt);
}

TEST_F (OpenCorruptStore, HeaderFooterTooLarge) {
    auto too_large = pstore::typed_address<pstore::trailer>::make (
        (UINT64_C (1) << (pstore::address::offset_number_bits +
                          pstore::address::segment_number_bits)) -
        1U);
    this->get_header ()->footer_pos = too_large;
    this->check_database_open (pstore::error_code::header_corrupt);
}



TEST_F (Database, SegmentBase) {
    // Checks that the first segment address is equal to the address of our file backing store
    // buffer, and that all of the other segment pointers are null.

    pstore::database db{this->file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    static_assert (pstore::address::segment_size == pstore::storage::min_region_size,
                   "expected min_region_size == segment_size");

    auto ptr = this->buffer ().get ();

    ASSERT_LE (std::numeric_limits<pstore::address::segment_type>::max (), pstore::sat_elements);

    EXPECT_EQ (std::shared_ptr<void> (this->buffer (), ptr), db.storage ().segment_base (0));
    for (unsigned segment = 1; segment < pstore::sat_elements; ++segment) {
        auto const si = static_cast<pstore::address::segment_type> (segment);
        EXPECT_EQ (nullptr, db.storage ().segment_base (si));
    }

    // Now the same tests, but for the const method.
    auto const * dbp = &db;

    EXPECT_EQ (std::shared_ptr<void> (this->buffer (), ptr), dbp->storage ().segment_base (0));
    for (unsigned segment = 1; segment < pstore::sat_elements; ++segment) {
        auto const si = static_cast<pstore::address::segment_type> (segment);
        EXPECT_EQ (nullptr, dbp->storage ().segment_base (si));
    }
}

TEST_F (Database, GetEndPastLogicalEOF) {
    pstore::database db{this->file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    auto const addr = pstore::address::null ();
    std::size_t size = db.size () + 1;
    check_for_error ([&db, addr, size]() { db.getro (addr, size); },
                     pstore::error_code::bad_address);
}

TEST_F (Database, GetStartPastLogicalEOF) {
    pstore::database db{this->file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    auto const addr = pstore::address::make (db.size () + 1);
    std::size_t size = 1;
    check_for_error ([&db, addr, size]() { db.getro (addr, size); },
                     pstore::error_code::bad_address);
}

TEST_F (Database, GetLocationOverflows) {
    pstore::database db{this->file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    auto const addr = std::numeric_limits<pstore::address>::max ();
    PSTORE_STATIC_ASSERT (std::numeric_limits<std::size_t>::max () >=
                          std::numeric_limits<std::uint64_t>::max ());
    std::size_t const size = std::numeric_limits<std::uint64_t>::max () - addr.absolute () + 1U;
    ASSERT_TRUE (addr + size < addr); // This addition is attended to overflow.
    check_for_error ([&db, addr, size]() { db.getro (addr, size); },
                     pstore::error_code::bad_address);
}



TEST_F (Database, Allocate16Bytes) {
    pstore::database db{this->file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    // Initial allocation.
    constexpr auto size = 16U;
    constexpr auto align = 1U;
    pstore::address addr = db.allocate (size, align);
    EXPECT_EQ (sizeof (pstore::header) + sizeof (pstore::trailer), addr.absolute ());

    // Subsequent allocation.
    pstore::address addr2 = db.allocate (size, align);
    EXPECT_EQ (addr.absolute () + 16, addr2.absolute ());
}

TEST_F (Database, Allocate16BytesAligned1024) {
    pstore::database db{this->file ()};
    db.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

    constexpr unsigned size = 16;
    constexpr unsigned align = 1024;
    PSTORE_STATIC_ASSERT (align > sizeof (pstore::header) + sizeof (pstore::trailer));

    pstore::address addr = db.allocate (size, align);
    EXPECT_EQ (0U, addr.absolute () % align);

    pstore::address addr2 = db.allocate (size, align);
    EXPECT_EQ (addr.absolute () + align, addr2.absolute ());
}
