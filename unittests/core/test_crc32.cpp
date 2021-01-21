//*                _________   *
//*   ___ _ __ ___|___ /___ \  *
//*  / __| '__/ __| |_ \ __) | *
//* | (__| | | (__ ___) / __/  *
//*  \___|_|  \___|____/_____| *
//*                            *
//===- unittests/core/test_crc32.cpp --------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/core/crc32.hpp"
#include <cstring>
#include "gtest/gtest.h"
#include "pstore/support/gsl.hpp"

TEST (Crc32, Empty) {
    char empty;
    EXPECT_EQ (pstore::crc32 (pstore::gsl::make_span (&empty, &empty)), ~0U);
}
TEST (Crc32, SingleChar) {
    char one_char[]{'a'};
    EXPECT_EQ (pstore::crc32 (pstore::gsl::make_span (one_char)), 0xc54aae31U);
}
TEST (Crc32, Sequence) {
    char const str[] = "hello";
    auto const length = static_cast<int> (std::strlen (str));
    auto span = pstore::gsl::make_span (str, length);
    EXPECT_EQ (pstore::crc32 (span), 0x0fcdae64U);
}
