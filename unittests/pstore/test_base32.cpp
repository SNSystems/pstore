//*  _                    _________   *
//* | |__   __ _ ___  ___|___ /___ \  *
//* | '_ \ / _` / __|/ _ \ |_ \ __) | *
//* | |_) | (_| \__ \  __/___) / __/  *
//* |_.__/ \__,_|___/\___|____/_____| *
//*                                   *
//===- unittests/pstore/test_base32.cpp -----------------------------------===//
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
/// \file test_base32.cpp
#include "base32.hpp"
#include "gtest/gtest.h"
#include "pstore/uint128.hpp"

TEST (Base32, Zero) {
    using namespace pstore::base32;
    EXPECT_EQ (convert (0U), "a");
    EXPECT_EQ (convert (::pstore::uint128 ()), "a");
}
TEST (Base32, TwentyFive) {
    using pstore::base32::convert;
    using pstore::uint128;
    EXPECT_EQ (convert (25U), "z");
    EXPECT_EQ (convert (uint128{25U}), "z");
}
TEST (Base32, TwentySix) {
    using pstore::base32::convert;
    using pstore::uint128;
    EXPECT_EQ (convert (26U), "2");
    EXPECT_EQ (convert (uint128{26U}), "2");
}
TEST (Base32, ThirtyOne) {
    using pstore::base32::convert;
    using pstore::uint128;
    EXPECT_EQ (convert (31U), "7");
    EXPECT_EQ (convert (uint128{31U}), "7");
}
TEST (Base32, ThirtyTwo) {
    using pstore::base32::convert;
    using pstore::uint128;
    EXPECT_EQ (convert (32U), "ab");
    EXPECT_EQ (convert (uint128{32U}), "ab");
}

TEST (Base32, UInt128MaxArray) {
    using pstore::base32::convert;
    using pstore::uint128;
    uint128 v{std::array<std::uint8_t, 16>{{
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff,
    }}};
    EXPECT_EQ (convert (v), "7777777777777777777777777h");
}
TEST (Base32, UInt128MaxTwoUint64s) {
    using pstore::base32::convert;
    using pstore::uint128;
    uint128 v{UINT64_C (0xffffffffffffffff), UINT64_C (0xffffffffffffffff)};
    EXPECT_EQ (convert (v), "7777777777777777777777777h");
}

TEST (Base32, UInt128High64Array) {
    using pstore::base32::convert;
    using pstore::uint128;
    uint128 v{std::array<std::uint8_t, 16>{{
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00,
    }}};
    EXPECT_EQ (convert (v), "aaaaaaaaaaaaq777777777777h");
}
TEST (Base32, UInt128High64TwoUint64s) {
    using pstore::base32::convert;
    using pstore::uint128;
    uint128 v{UINT64_C (0xffffffffffffffff), 0U};
    EXPECT_EQ (convert (v), "aaaaaaaaaaaaq777777777777h");
}

TEST (Base32, UInt128Low64Array) {
    using pstore::base32::convert;
    using pstore::uint128;
    uint128 v{std::array<std::uint8_t, 16>{{
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff,
    }}};
    EXPECT_EQ (convert (v), "777777777777p");
}
TEST (Base32, UInt128Low64TwoUint64s) {
    using pstore::base32::convert;
    using pstore::uint128;
    uint128 v{0U, UINT64_C (0xffffffffffffffff)};
    EXPECT_EQ (convert (v), "777777777777p");
}


TEST (Base32, UInt128TopBitTwoUint64s) {
    using pstore::base32::convert;
    using pstore::uint128;
    uint128 v{UINT64_C (0x8000000000000000), 0U};
    EXPECT_EQ (convert (v), "aaaaaaaaaaaaaaaaaaaaaaaaae");
}
// eof: unittests/pstore/test_base32.cpp
