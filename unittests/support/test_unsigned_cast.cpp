//*                  _                      _                 _    *
//*  _   _ _ __  ___(_) __ _ _ __   ___  __| |   ___ __ _ ___| |_  *
//* | | | | '_ \/ __| |/ _` | '_ \ / _ \/ _` |  / __/ _` / __| __| *
//* | |_| | | | \__ \ | (_| | | | |  __/ (_| | | (_| (_| \__ \ |_  *
//*  \__,_|_| |_|___/_|\__, |_| |_|\___|\__,_|  \___\__,_|___/\__| *
//*                    |___/                                       *
//===- unittests/support/test_unsigned_cast.cpp ---------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/support/unsigned_cast.hpp"

#include <gtest/gtest.h>

#include "check_for_error.hpp"

TEST (UnsignedCast, Zero) {
    EXPECT_EQ (pstore::checked_unsigned_cast (0), 0U);
}

TEST (UnsignedCast, Max) {
    auto const max = std::numeric_limits<int>::max ();
    EXPECT_EQ (pstore::checked_unsigned_cast (max), static_cast<unsigned> (max));
}

TEST (UnsignedCast, Negative) {
    check_for_error ([] () { pstore::checked_unsigned_cast (-1); }, std::errc::invalid_argument);
}
