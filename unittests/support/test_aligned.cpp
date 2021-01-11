//*        _ _                      _  *
//*   __ _| (_) __ _ _ __   ___  __| | *
//*  / _` | | |/ _` | '_ \ / _ \/ _` | *
//* | (_| | | | (_| | | | |  __/ (_| | *
//*  \__,_|_|_|\__, |_| |_|\___|\__,_| *
//*            |___/                   *
//===- unittests/support/test_aligned.cpp ---------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
#include "pstore/support/aligned.hpp"
#include <gtest/gtest.h>

TEST (Aligned, IsPowerOfTwo) {
    EXPECT_FALSE (pstore::is_power_of_two (0U));
    EXPECT_TRUE (pstore::is_power_of_two (1U));
    EXPECT_TRUE (pstore::is_power_of_two (2U));
    EXPECT_FALSE (pstore::is_power_of_two (3U));
    EXPECT_TRUE (pstore::is_power_of_two (4U));
    EXPECT_FALSE (pstore::is_power_of_two (255U));
    EXPECT_TRUE (pstore::is_power_of_two (256U));
}

TEST (Aligned, Aligned1) {
    EXPECT_EQ (pstore::aligned (0U, 1U), 0U);
    EXPECT_EQ (pstore::aligned (1U, 1U), 1U);
    EXPECT_EQ (pstore::aligned (2U, 1U), 2U);
    EXPECT_EQ (pstore::aligned (3U, 1U), 3U);
}

TEST (Aligned, Aligned2) {
    EXPECT_EQ (pstore::aligned (0U, 2U), 0U);
    EXPECT_EQ (pstore::aligned (1U, 2U), 2U);
    EXPECT_EQ (pstore::aligned (2U, 2U), 2U);
    EXPECT_EQ (pstore::aligned (3U, 2U), 4U);
    EXPECT_EQ (pstore::aligned (4U, 2U), 4U);
}

TEST (Aligned, AlignedUnsigned) {
    EXPECT_EQ (pstore::aligned<unsigned> (0U), 0U);
    constexpr auto unsigned_align = static_cast<unsigned> (alignof (unsigned));
    if (unsigned_align >= 2) {
        EXPECT_EQ (pstore::aligned<unsigned> (1U), unsigned_align);
        EXPECT_EQ (pstore::aligned<unsigned> (2U), unsigned_align);
    }
    if (unsigned_align >= 4) {
        EXPECT_EQ (pstore::aligned<unsigned> (3U), unsigned_align);
        EXPECT_EQ (pstore::aligned<unsigned> (4U), unsigned_align);
    }
    if (unsigned_align >= 8) {
        EXPECT_EQ (pstore::aligned<unsigned> (5U), unsigned_align);
        EXPECT_EQ (pstore::aligned<unsigned> (6U), unsigned_align);
        EXPECT_EQ (pstore::aligned<unsigned> (7U), unsigned_align);
        EXPECT_EQ (pstore::aligned<unsigned> (8U), unsigned_align);
    }
}
