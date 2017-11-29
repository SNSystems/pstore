//*      _                        _                                              *
//*  ___| |__   __ _ _ __ ___  __| |  _ __ ___   ___ _ __ ___   ___  _ __ _   _  *
//* / __| '_ \ / _` | '__/ _ \/ _` | | '_ ` _ \ / _ \ '_ ` _ \ / _ \| '__| | | | *
//* \__ \ | | | (_| | | |  __/ (_| | | | | | | |  __/ | | | | | (_) | |  | |_| | *
//* |___/_| |_|\__,_|_|  \___|\__,_| |_| |_| |_|\___|_| |_| |_|\___/|_|   \__, | *
//*                                                                       |___/  *
//===- unittests/pstore/test_shared_memory.cpp ----------------------------===//
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
/// \file test_shared_memory.cpp

#include "pstore/shared_memory.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include "gmock/gmock.h"
#include "pstore_support/gsl.hpp"

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
    EXPECT_TRUE (std::all_of (begin, std::end (arr), [sentinel](char c) { return c == sentinel; }));
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

// eof: unittests/pstore/test_shared_memory.cpp
