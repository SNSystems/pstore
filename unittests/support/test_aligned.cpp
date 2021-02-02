//*        _ _                      _  *
//*   __ _| (_) __ _ _ __   ___  __| | *
//*  / _` | | |/ _` | '_ \ / _ \/ _` | *
//* | (_| | | | (_| | | | |  __/ (_| | *
//*  \__,_|_|_|\__, |_| |_|\___|\__,_| *
//*            |___/                   *
//===- unittests/support/test_aligned.cpp ---------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
