//*  _     _ _                           _    *
//* | |__ (_) |_    ___ ___  _   _ _ __ | |_  *
//* | '_ \| | __|  / __/ _ \| | | | '_ \| __| *
//* | |_) | | |_  | (_| (_) | |_| | | | | |_  *
//* |_.__/|_|\__|  \___\___/ \__,_|_| |_|\__| *
//*                                           *
//===- unittests/support/test_bit_count.cpp -------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/support/bit_count.hpp"
#include "gmock/gmock.h"

TEST (BitCount, CountLeadingZeros) {
    EXPECT_EQ (127U, pstore::bit_count::clz (pstore::uint128{1U}));
    EXPECT_EQ (126U, pstore::bit_count::clz (pstore::uint128{2U}));
    EXPECT_EQ (0U, pstore::bit_count::clz (std::numeric_limits<pstore::uint128>::max ()));

    EXPECT_EQ (63U, pstore::bit_count::clz (std::uint64_t{1}));
    EXPECT_EQ (62U, pstore::bit_count::clz (std::uint64_t{2}));
    EXPECT_EQ (0U, pstore::bit_count::clz (std::numeric_limits<std::uint64_t>::max ()));

    EXPECT_EQ (31U, pstore::bit_count::clz (std::uint32_t{1}));
    EXPECT_EQ (30U, pstore::bit_count::clz (std::uint32_t{2}));
    EXPECT_EQ (0U, pstore::bit_count::clz (std::numeric_limits<std::uint32_t>::max ()));

    EXPECT_EQ (15U, pstore::bit_count::clz (std::uint16_t{1}));
    EXPECT_EQ (14U, pstore::bit_count::clz (std::uint16_t{2}));
    EXPECT_EQ (0U, pstore::bit_count::clz (std::numeric_limits<std::uint16_t>::max ()));

    EXPECT_EQ (7U, pstore::bit_count::clz (std::uint8_t{1}));
    EXPECT_EQ (6U, pstore::bit_count::clz (std::uint8_t{2}));
    EXPECT_EQ (0U, pstore::bit_count::clz (std::numeric_limits<std::uint8_t>::max ()));
}

TEST (BitCount, CountTrailingZeros) {
    EXPECT_EQ (127U, pstore::bit_count::ctz (pstore::uint128{std::uint64_t{1} << 63, 0U}));
    EXPECT_EQ (63U, pstore::bit_count::ctz (pstore::uint128{0U, std::uint64_t{1} << 63}));

    EXPECT_EQ (63U, pstore::bit_count::ctz (std::uint64_t{1} << 63));
    EXPECT_EQ (0U, pstore::bit_count::ctz (std::uint64_t{1}));
    EXPECT_EQ (1U, pstore::bit_count::ctz (std::uint64_t{2}));
    EXPECT_EQ (0U, pstore::bit_count::ctz (std::numeric_limits<std::uint64_t>::max ()));
}


TEST (BitCount, PopCountUChar) {
    std::uint8_t bitmap = 0;
    EXPECT_EQ (0U, pstore::bit_count::pop_count (bitmap));
    bitmap = 0xF0;
    EXPECT_EQ (4U, pstore::bit_count::pop_count (bitmap));
    bitmap = 0xFF;
    EXPECT_EQ (8U, pstore::bit_count::pop_count (bitmap));
}

TEST (BitCount, PopCountUShort) {
    std::uint16_t bitmap = 0;
    EXPECT_EQ (0U, pstore::bit_count::pop_count (bitmap));
    bitmap = 0xF0;
    EXPECT_EQ (4U, pstore::bit_count::pop_count (bitmap));
    bitmap = 0xFFFF;
    EXPECT_EQ (16U, pstore::bit_count::pop_count (bitmap));
}

TEST (BitCount, PopCountUInt) {
    std::uint32_t bitmap = 0;
    EXPECT_EQ (0U, pstore::bit_count::pop_count (bitmap));
    bitmap = 0xF0;
    EXPECT_EQ (4U, pstore::bit_count::pop_count (bitmap));
    bitmap = 0xFFFFFFFF;
    EXPECT_EQ (32U, pstore::bit_count::pop_count (bitmap));
}

TEST (BitCount, PopCountULong) {
    unsigned long bitmap = 0;
    EXPECT_EQ (0U, pstore::bit_count::pop_count (bitmap));
    bitmap = 0xF0;
    EXPECT_EQ (4U, pstore::bit_count::pop_count (bitmap));
    bitmap = 0xFFFFFFFF;
    EXPECT_EQ (32U, pstore::bit_count::pop_count (bitmap));
}

TEST (BitCount, PopCountULongLong) {
    std::uint64_t bitmap = 0;
    EXPECT_EQ (0U, pstore::bit_count::pop_count (bitmap));
    bitmap = 0xFF00;
    EXPECT_EQ (8U, pstore::bit_count::pop_count (bitmap));
    bitmap = 0xFFFFFFFFFFFFFFFF;
    EXPECT_EQ (64U, pstore::bit_count::pop_count (bitmap));
}
