//*            _     _                    *
//*   __ _  __| | __| |_ __ ___  ___ ___  *
//*  / _` |/ _` |/ _` | '__/ _ \/ __/ __| *
//* | (_| | (_| | (_| | | |  __/\__ \__ \ *
//*  \__,_|\__,_|\__,_|_|  \___||___/___/ *
//*                                       *
//===- unittests/core/test_address.cpp ------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file test_address.cpp

#include "pstore/core/address.hpp"

#include <gtest/gtest.h>

namespace {

    class AddressFixture : public ::testing::Test {
    public:
        pstore::address expected_address (std::uint64_t segment, std::uint64_t offset) const {
            return pstore::address{(segment << pstore::address::offset_number_bits) | offset};
        }
    };

} // end anonymous namespace


TEST_F (AddressFixture, InitNull) {
    auto addr = pstore::address::null ();
    EXPECT_EQ (0U, addr.absolute ());
    EXPECT_EQ (0U, addr.offset ());
    EXPECT_EQ (0U, addr.segment ());
}

TEST_F (AddressFixture, InitSegment0Offset1) {
    constexpr auto segment = pstore::address::segment_type{0};
    constexpr auto offset = pstore::address::offset_type{1};

    auto const expected = this->expected_address (segment, offset);
    auto const actual = pstore::address{segment, offset};

    EXPECT_EQ (expected, actual);
    EXPECT_EQ (expected.absolute (), actual.absolute ());
    EXPECT_EQ (segment, actual.segment ());
    EXPECT_EQ (offset, actual.offset ());
}

TEST_F (AddressFixture, InitSegment0OffsetMax) {
    constexpr auto segment = pstore::address::segment_type{0};
    constexpr auto offset = pstore::address::max_offset;

    auto const expected = this->expected_address (segment, offset);
    constexpr auto actual = pstore::address{segment, offset};

    EXPECT_EQ (expected, actual);
    EXPECT_EQ (expected.absolute (), actual.absolute ());
    EXPECT_EQ (segment, actual.segment ());
    EXPECT_EQ (offset, actual.offset ());
}

TEST_F (AddressFixture, InitSegment1Offset0) {
    constexpr auto segment = pstore::address::segment_type{1};
    constexpr auto offset = pstore::address::offset_type{0};

    auto const expected = this->expected_address (segment, offset);
    constexpr auto actual = pstore::address{segment, offset};

    EXPECT_EQ (expected, actual);
    EXPECT_EQ (expected.absolute (), actual.absolute ());
    EXPECT_EQ (segment, actual.segment ());
    EXPECT_EQ (offset, actual.offset ());
}

TEST_F (AddressFixture, InitSegmentMaxOffsetMax) {
    constexpr auto segment = pstore::address::max_segment;
    constexpr auto offset = pstore::address::max_offset;

    auto const expected = this->expected_address (segment, offset);
    auto const actual = pstore::address{segment, offset};

    EXPECT_EQ (expected, actual);
    EXPECT_EQ (expected.absolute (), actual.absolute ());
    EXPECT_EQ (segment, actual.segment ());
    EXPECT_EQ (offset, actual.offset ());
}

TEST_F (AddressFixture, Address0Plus1) {
    constexpr auto addr = pstore::address::null ();
    pstore::address actual = addr + 1;
    EXPECT_EQ (pstore::address{1}, actual);
}

TEST_F (AddressFixture, Address0PlusEqual1) {
    auto addr = pstore::address::null ();
    addr += 1;
    EXPECT_EQ (pstore::address{1}, addr);
}

TEST_F (AddressFixture, Address0PlusOffsetMax) {
    constexpr auto addr = pstore::address::null ();
    constexpr auto increment = pstore::address::max_offset;
    pstore::address actual = addr + increment;
    EXPECT_EQ (pstore::address{increment}, actual);
    EXPECT_EQ (0U, actual.segment ());
    EXPECT_EQ (pstore::address::max_offset, actual.offset ());
}

TEST_F (AddressFixture, Address0PlusOffsetMaxPlus1) {
    auto addr = pstore::address::null ();
    constexpr auto increment = pstore::address::max_offset + 1;
    pstore::address actual = addr + increment;
    EXPECT_EQ (pstore::address{increment}, actual);
    EXPECT_EQ (1U, actual.segment ());
    EXPECT_EQ (0U, actual.offset ());
}

TEST_F (AddressFixture, AddressMaxOffsetPlus1) {
    constexpr auto addr = pstore::address{pstore::address::max_offset};
    pstore::address const actual = addr + 1;
    auto const expected = pstore::address{1, 0};
    EXPECT_EQ (expected, actual);
    EXPECT_EQ (1U, actual.segment ());
    EXPECT_EQ (0U, actual.offset ());
}


TEST_F (AddressFixture, NotEqual) {
    constexpr auto zero = pstore::address::null ();
    constexpr auto one = pstore::address{1};
    EXPECT_NE (zero, one);
}


TEST_F (AddressFixture, Address1Minus1) {
    constexpr auto const addr = pstore::address{1};
    pstore::address const actual = addr - 1;
    EXPECT_EQ (pstore::address::null (), actual);
}

TEST_F (AddressFixture, Decrement) {
    {
        // Pre-decrement
        auto a1 = pstore::address{1};
        auto r1 = --a1;
        EXPECT_EQ (pstore::address::null (), a1);
        EXPECT_EQ (pstore::address::null (), r1);
    }
    {
        // Post-decrement
        auto a2 = pstore::address{1};
        auto r2 = a2--;
        EXPECT_EQ (pstore::address::null (), a2);
        EXPECT_EQ ((pstore::address{1}), r2);
    }
}

TEST_F (AddressFixture, Increment) {
    {
        // Pre-increment
        auto a1 = pstore::address{1};
        auto r1 = ++a1;
        EXPECT_EQ ((pstore::address{2}), a1);
        EXPECT_EQ ((pstore::address{2}), r1);
    }
    {
        // Post-increment
        auto a2 = pstore::address{1};
        auto r2 = a2++;
        EXPECT_EQ (pstore::address{2}, a2);
        EXPECT_EQ (pstore::address{1}, r2);
    }
}

TEST_F (AddressFixture, Address0MinusEqual1) {
    auto addr = pstore::address{1U};
    addr -= 1;
    EXPECT_EQ (pstore::address::null (), addr);
}

TEST_F (AddressFixture, AddressSegment1Minus1) {
    // {segment: 1 offset: 0} minus 1 equals {segment: 0, offset: max_offset}
    constexpr auto addr = pstore::address{1, 0};
    pstore::address const actual = addr - 1;
    EXPECT_EQ ((pstore::address{0, pstore::address::max_offset}), actual);
}

TEST_F (AddressFixture, AddressSegment1MinusEqual1) {
    // {segment: 1 offset: 0} minus 1 equals {segment: 0, offset: max_offset}
    auto actual = pstore::address{1, 0};
    actual -= 1;
    EXPECT_EQ ((pstore::address{0, pstore::address::max_offset}), actual);
}

TEST (TypedAddress, Decrement) {
    {
        // Pre-decrement
        auto a1 = pstore::typed_address<std::uint64_t>::make (sizeof (std::uint64_t));
        auto r1 = --a1;
        EXPECT_EQ (pstore::typed_address<std::uint64_t>::null (), a1);
        EXPECT_EQ (pstore::typed_address<std::uint64_t>::null (), r1);
    }
    {
        // Post-decrement.
        auto a2 = pstore::typed_address<std::uint64_t>::make (sizeof (std::uint64_t));
        auto r2 = a2--;
        EXPECT_EQ (pstore::typed_address<std::uint64_t>::null (), a2);
        EXPECT_EQ (pstore::typed_address<std::uint64_t>::make (sizeof (std::uint64_t)), r2);
    }
}

TEST (TypedAddress, Increment) {
    {
        // Pre-increment
        auto a1 = pstore::typed_address<std::uint64_t>::null ();
        auto r1 = ++a1;
        EXPECT_EQ (pstore::typed_address<std::uint64_t>::make (sizeof (std::uint64_t)), a1);
        EXPECT_EQ (pstore::typed_address<std::uint64_t>::make (sizeof (std::uint64_t)), r1);
    }
    {
        // Post-increment.
        auto a2 = pstore::typed_address<std::uint64_t>::null ();
        auto r2 = a2++;
        EXPECT_EQ (pstore::typed_address<std::uint64_t>::make (sizeof (std::uint64_t)), a2);
        EXPECT_EQ (pstore::typed_address<std::uint64_t>::null (), r2);
    }
}

TEST (Extent, ComparisonOperators) {
    {
        auto const extent1 =
            make_extent (pstore::typed_address<std::uint8_t>::make (2), UINT64_C (4));
        auto const extent2 =
            make_extent (pstore::typed_address<std::uint8_t>::make (2), UINT64_C (4));

        EXPECT_TRUE (extent1 == extent2);
        EXPECT_TRUE (!(extent1 != extent2));
        EXPECT_TRUE (!(extent1 < extent2));
        EXPECT_TRUE (extent1 <= extent2);
        EXPECT_TRUE (!(extent1 > extent2));
        EXPECT_TRUE (extent1 >= extent2);
        EXPECT_TRUE (extent1 == extent1);
        EXPECT_TRUE (!(extent2 != extent1));
        EXPECT_TRUE (!(extent2 < extent1));
        EXPECT_TRUE (extent2 <= extent1);
        EXPECT_TRUE (!(extent2 > extent1));
        EXPECT_TRUE (extent2 >= extent1);
    }
    {
        auto const extent1 =
            make_extent (pstore::typed_address<std::uint8_t>::make (2), UINT64_C (4));
        auto const extent2 =
            make_extent (pstore::typed_address<std::uint8_t>::make (5), UINT64_C (4)); // bigger

        EXPECT_TRUE (extent1 != extent2);
        EXPECT_TRUE (extent2 != extent1);
        EXPECT_FALSE (extent1 == extent2);
        EXPECT_FALSE (extent2 == extent1);
        EXPECT_TRUE (extent1 < extent2);
        EXPECT_FALSE (extent2 < extent1);
        EXPECT_TRUE (extent1 <= extent2);
        EXPECT_TRUE (!(extent2 <= extent1));
        EXPECT_TRUE (extent2 > extent1);
        EXPECT_TRUE (!(extent1 > extent2));
        EXPECT_TRUE (extent2 >= extent1);
        EXPECT_TRUE (!(extent1 >= extent2));
    }
    {
        auto const extent1 =
            make_extent (pstore::typed_address<std::uint8_t>::make (2), UINT64_C (4));
        auto const extent2 =
            make_extent (pstore::typed_address<std::uint8_t>::make (2), UINT64_C (5)); // bigger

        EXPECT_TRUE (extent1 != extent2);
        EXPECT_TRUE (extent2 != extent1);
        EXPECT_FALSE (extent1 == extent2);
        EXPECT_FALSE (extent2 == extent1);
        EXPECT_TRUE (extent1 < extent2);
        EXPECT_FALSE (extent2 < extent1);
        EXPECT_TRUE (extent1 <= extent2);
        EXPECT_TRUE (!(extent2 <= extent1));
        EXPECT_TRUE (extent2 > extent1);
        EXPECT_TRUE (!(extent1 > extent2));
        EXPECT_TRUE (extent2 >= extent1);
        EXPECT_TRUE (!(extent1 >= extent2));
    }
}
