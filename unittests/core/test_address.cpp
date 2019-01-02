//*            _     _                    *
//*   __ _  __| | __| |_ __ ___  ___ ___  *
//*  / _` |/ _` |/ _` | '__/ _ \/ __/ __| *
//* | (_| | (_| | (_| | | |  __/\__ \__ \ *
//*  \__,_|\__,_|\__,_|_|  \___||___/___/ *
//*                                       *
//===- unittests/core/test_address.cpp ------------------------------------===//
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
/// \file test_address.cpp

#include "pstore/core/address.hpp"

#include "gtest/gtest.h"

namespace {

    class AddressFixture : public ::testing::Test {
    public:
        pstore::address expected_address (std::uint64_t segment, std::uint64_t offset) const {
            return pstore::address::make ((segment << pstore::address::offset_number_bits) |
                                          offset);
        }

        static constexpr pstore::address::offset_type max_offset = pstore::address::max_offset;
        static constexpr pstore::address::segment_type max_segment = pstore::address::max_segment;
    };

    constexpr pstore::address::offset_type AddressFixture::max_offset;
    constexpr pstore::address::segment_type AddressFixture::max_segment;

} // namespace


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
    auto const actual = pstore::address::make (segment, offset);

    EXPECT_EQ (expected, actual);
    EXPECT_EQ (expected.absolute (), actual.absolute ());
    EXPECT_EQ (segment, actual.segment ());
    EXPECT_EQ (offset, actual.offset ());
}

TEST_F (AddressFixture, InitSegment0OffsetMax) {
    auto const segment = pstore::address::segment_type{0};
    auto const offset = this->max_offset;

    auto const expected = this->expected_address (segment, offset);
    auto const actual = pstore::address::make (segment, offset);

    EXPECT_EQ (expected, actual);
    EXPECT_EQ (expected.absolute (), actual.absolute ());
    EXPECT_EQ (segment, actual.segment ());
    EXPECT_EQ (offset, actual.offset ());
}

TEST_F (AddressFixture, InitSegment1Offset0) {
    constexpr auto segment = pstore::address::segment_type{1};
    constexpr auto offset = pstore::address::offset_type{0};

    auto const expected = this->expected_address (segment, offset);
    auto const actual = pstore::address::make (segment, offset);

    EXPECT_EQ (expected, actual);
    EXPECT_EQ (expected.absolute (), actual.absolute ());
    EXPECT_EQ (segment, actual.segment ());
    EXPECT_EQ (offset, actual.offset ());
}

TEST_F (AddressFixture, InitSegmentMaxOffsetMax) {
    auto const segment = this->max_segment;
    auto const offset = this->max_offset;

    auto const expected = this->expected_address (segment, offset);
    auto const actual = pstore::address::make (segment, offset);

    EXPECT_EQ (expected, actual);
    EXPECT_EQ (expected.absolute (), actual.absolute ());
    EXPECT_EQ (segment, actual.segment ());
    EXPECT_EQ (offset, actual.offset ());
}

TEST_F (AddressFixture, Address0Plus1) {
    auto addr = pstore::address::null ();
    pstore::address actual = addr + 1;
    EXPECT_EQ (pstore::address::make (1), actual);
}

TEST_F (AddressFixture, Address0PlusEqual1) {
    auto addr = pstore::address::null ();
    addr += 1;
    EXPECT_EQ (pstore::address::make (1), addr);
}

TEST_F (AddressFixture, Address0PlusOffsetMax) {
    auto addr = pstore::address::null ();
    constexpr auto increment = max_offset;
    pstore::address actual = addr + increment;
    EXPECT_EQ (pstore::address::make (increment), actual);
    EXPECT_EQ (0U, actual.segment ());
    EXPECT_EQ (this->max_offset, actual.offset ());
}

TEST_F (AddressFixture, Address0PlusOffsetMaxPlus1) {
    auto addr = pstore::address::null ();
    constexpr auto increment = max_offset + 1;
    pstore::address actual = addr + increment;
    EXPECT_EQ (pstore::address::make (increment), actual);
    EXPECT_EQ (1U, actual.segment ());
    EXPECT_EQ (0U, actual.offset ());
}

TEST_F (AddressFixture, AddressMaxOffsetPlus1) {
    auto addr = pstore::address::make (max_offset);
    pstore::address const actual = addr + 1;
    auto const expected = pstore::address::make (1, 0);
    EXPECT_EQ (expected, actual);
    EXPECT_EQ (1U, actual.segment ());
    EXPECT_EQ (0U, actual.offset ());
}


TEST_F (AddressFixture, NotEqual) {
    auto zero = pstore::address::null ();
    auto one = pstore::address::make (1);
    EXPECT_NE (zero, one);
}


TEST_F (AddressFixture, Address1Minus1) {
    auto const addr = pstore::address::make (1);
    pstore::address const actual = addr - 1;
    EXPECT_EQ (pstore::address::null (), actual);
}

TEST_F (AddressFixture, Decrement) {
    {
        // Pre-decrement
        auto a1 = pstore::address::make (1);
        auto r1 = --a1;
        EXPECT_EQ (pstore::address::null (), a1);
        EXPECT_EQ (pstore::address::null (), r1);
    }
    {
        // Post-decrement
        auto a2 = pstore::address::make (1);
        auto r2 = a2--;
        EXPECT_EQ (pstore::address::null (), a2);
        EXPECT_EQ (pstore::address::make (1), r2);
    }
}

TEST_F (AddressFixture, Increment) {
    {
        // Pre-increment
        auto a1 = pstore::address::make (1);
        auto r1 = ++a1;
        EXPECT_EQ (pstore::address::make (2), a1);
        EXPECT_EQ (pstore::address::make (2), r1);
    }
    {
        // Post-increment
        auto a2 = pstore::address::make (1);
        auto r2 = a2++;
        EXPECT_EQ (pstore::address::make (2), a2);
        EXPECT_EQ (pstore::address::make (1), r2);
    }
}


TEST_F (AddressFixture, Address0MinusEqual1) {
    auto addr = pstore::address::make (1U);
    addr -= 1;
    EXPECT_EQ (pstore::address::null (), addr);
}

TEST_F (AddressFixture, AddressSegment1Minus1) {
    // {segment: 1 offset: 0} minus 1 equals {segment: 0, offset: max_offset}
    auto const addr = pstore::address::make (1, 0);
    pstore::address const actual = addr - 1;
    auto const expected = pstore::address::make (0, max_offset);
    EXPECT_EQ (expected, actual);
}

TEST_F (AddressFixture, AddressSegment1MinusEqual1) {
    // {segment: 1 offset: 0} minus 1 equals {segment: 0, offset: max_offset}
    auto actual = pstore::address::make (1, 0);
    actual -= 1;
    auto const expected = pstore::address::make (0, max_offset);
    EXPECT_EQ (expected, actual);
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
