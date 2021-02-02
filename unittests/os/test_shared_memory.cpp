//*      _                        _                                              *
//*  ___| |__   __ _ _ __ ___  __| |  _ __ ___   ___ _ __ ___   ___  _ __ _   _  *
//* / __| '_ \ / _` | '__/ _ \/ _` | | '_ ` _ \ / _ \ '_ ` _ \ / _ \| '__| | | | *
//* \__ \ | | | (_| | | |  __/ (_| | | | | | | |  __/ | | | | | (_) | |  | |_| | *
//* |___/_| |_|\__,_|_|  \___|\__,_| |_| |_| |_|\___|_| |_| |_|\___/|_|   \__, | *
//*                                                                       |___/  *
//===- unittests/os/test_shared_memory.cpp --------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file test_shared_memory.cpp

#include "pstore/os/shared_memory.hpp"

// Stadard library
#include <algorithm>
#include <array>
#include <limits>
// 3rd party
#include <gmock/gmock.h>
// pstore
#include "pstore/support/gsl.hpp"

TEST (PosixMutexName, LargeOutputBuffer) {
    std::array<char, 256> arr;

    // Flood the array with known values so that we can detect change in any unexepected elements.

    auto const sentinel = std::numeric_limits<char>::max ();
    arr.fill (sentinel);

    // Now the real work of the test.

    std::string const expected = "/name";
    char const * actual = pstore::posix::shm_name ("name", arr);
    EXPECT_THAT (actual, ::testing::StrEq (expected));

    // Check that none of the remaining values in arr was modified. (The plus one
    // in the initialization of begin is to allow for the czstring's terminating null
    // character.)

    auto const begin = std::begin (arr) + expected.length () + 1;
    EXPECT_TRUE (
        std::all_of (begin, std::end (arr), [sentinel] (char c) { return c == sentinel; }));
}

TEST (PosixMutexName, OutputBufferTooSmall) {
    std::array<char, 4> arr;
    char const * actual = pstore::posix::shm_name ("name", arr);
    EXPECT_THAT (actual, ::testing::StrEq ("/na"));
}

TEST (PosixMutexName, OutputBufferTooSmallExplicitSpan) {
    std::array<char, 4> arr;
    char const * actual = pstore::posix::shm_name ("name", ::pstore::gsl::make_span (arr));
    EXPECT_THAT (actual, ::testing::StrEq ("/na"));
}

TEST (PosixMutexName, OutputBufferFilled) {
    std::array<char, 6> arr;
    char const * actual = pstore::posix::shm_name ("name", arr);
    EXPECT_THAT (actual, ::testing::StrEq ("/name"));
}

TEST (PosixMutexName, MinimumSizeOutputBuffer) {
    std::array<char, 2> arr;
    char const * actual = pstore::posix::shm_name ("name", arr);
    EXPECT_THAT (actual, ::testing::StrEq ("/"));
}
