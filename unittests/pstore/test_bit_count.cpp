//*  _     _ _                           _    *
//* | |__ (_) |_    ___ ___  _   _ _ __ | |_  *
//* | '_ \| | __|  / __/ _ \| | | | '_ \| __| *
//* | |_) | | |_  | (_| (_) | |_| | | | | |_  *
//* |_.__/|_|\__|  \___\___/ \__,_|_| |_|\__| *
//*                                           *
//===- unittests/pstore/test_bit_count.cpp --------------------------------===//
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
#include "pstore/bit_count.hpp"
#include "gmock/gmock.h"

TEST (BitCount, CountLeadingZeros) {
    EXPECT_EQ (63U, pstore::bit_count::clz (std::uint64_t{1}));
    EXPECT_EQ (62U, pstore::bit_count::clz (std::uint64_t{2}));
    EXPECT_EQ (0U, pstore::bit_count::clz (std::numeric_limits<std::uint64_t>::max ()));
}

TEST (BitCount, CountTrailingZeros) {
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
// eof: unittests/pstore/test_bit_count.cpp
