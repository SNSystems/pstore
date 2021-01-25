//*  _                    _________   *
//* | |__   __ _ ___  ___|___ /___ \  *
//* | '_ \ / _` / __|/ _ \ |_ \ __) | *
//* | |_) | (_| \__ \  __/___) / __/  *
//* |_.__/ \__,_|___/\___|____/_____| *
//*                                   *
//===- unittests/core/test_base32.cpp -------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file test_base32.cpp
#include "base32.hpp"
#include "gtest/gtest.h"
#include "pstore/support/uint128.hpp"

TEST (Base32, Zero) {
    using namespace pstore::base32;
    EXPECT_EQ (convert (0U), "a");
    EXPECT_EQ (convert (::pstore::uint128 ()), "a");
}
TEST (Base32, TwentyFive) {
    using pstore::uint128;
    using pstore::base32::convert;
    EXPECT_EQ (convert (25U), "z");
    EXPECT_EQ (convert (uint128{25U}), "z");
}
TEST (Base32, TwentySix) {
    using pstore::uint128;
    using pstore::base32::convert;
    EXPECT_EQ (convert (26U), "2");
    EXPECT_EQ (convert (uint128{26U}), "2");
}
TEST (Base32, ThirtyOne) {
    using pstore::uint128;
    using pstore::base32::convert;
    EXPECT_EQ (convert (31U), "7");
    EXPECT_EQ (convert (uint128{31U}), "7");
}
TEST (Base32, ThirtyTwo) {
    using pstore::uint128;
    using pstore::base32::convert;
    EXPECT_EQ (convert (32U), "ab");
    EXPECT_EQ (convert (uint128{32U}), "ab");
}

TEST (Base32, UInt128MaxArray) {
    using pstore::uint128;
    using pstore::base32::convert;
    uint128 v{std::array<std::uint8_t, 16>{{
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
    }}};
    EXPECT_EQ (convert (v), "7777777777777777777777777h");
}
TEST (Base32, UInt128MaxTwoUint64s) {
    using pstore::uint128;
    using pstore::base32::convert;
    uint128 v{UINT64_C (0xffffffffffffffff), UINT64_C (0xffffffffffffffff)};
    EXPECT_EQ (convert (v), "7777777777777777777777777h");
}

TEST (Base32, UInt128High64Array) {
    using pstore::uint128;
    using pstore::base32::convert;
    uint128 v{std::array<std::uint8_t, 16>{{
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    }}};
    EXPECT_EQ (convert (v), "aaaaaaaaaaaaq777777777777h");
}
TEST (Base32, UInt128High64TwoUint64s) {
    using pstore::uint128;
    using pstore::base32::convert;
    uint128 v{UINT64_C (0xffffffffffffffff), 0U};
    EXPECT_EQ (convert (v), "aaaaaaaaaaaaq777777777777h");
}

TEST (Base32, UInt128Low64Array) {
    using pstore::uint128;
    using pstore::base32::convert;
    uint128 v{std::array<std::uint8_t, 16>{{
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
    }}};
    EXPECT_EQ (convert (v), "777777777777p");
}
TEST (Base32, UInt128Low64TwoUint64s) {
    using pstore::uint128;
    using pstore::base32::convert;
    uint128 v{0U, UINT64_C (0xffffffffffffffff)};
    EXPECT_EQ (convert (v), "777777777777p");
}


TEST (Base32, UInt128TopBitTwoUint64s) {
    using pstore::uint128;
    using pstore::base32::convert;
    uint128 v{UINT64_C (0x8000000000000000), 0U};
    EXPECT_EQ (convert (v), "aaaaaaaaaaaaaaaaaaaaaaaaae");
}
