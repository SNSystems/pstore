//*      _        _                   _ _     _                        *
//*  ___| |_ _ __(_)_ __   __ _    __| (_)___| |_ __ _ _ __   ___ ___  *
//* / __| __| '__| | '_ \ / _` |  / _` | / __| __/ _` | '_ \ / __/ _ \ *
//* \__ \ |_| |  | | | | | (_| | | (_| | \__ \ || (_| | | | | (_|  __/ *
//* |___/\__|_|  |_|_| |_|\__, |  \__,_|_|___/\__\__,_|_| |_|\___\___| *
//*                       |___/                                        *
//===- unittests/command_line/test_string_distance.cpp --------------------===//
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
#include "pstore/command_line/string_distance.hpp"

#include "gtest/gtest.h"

using pstore::command_line::string_distance;
using namespace std::string_literals;

TEST (StringDistance, EmptyStrings) {
    EXPECT_EQ (string_distance (""s, ""s), 0U);
    EXPECT_EQ (string_distance ("a"s, ""s), 1U);
    EXPECT_EQ (string_distance (""s, "a"s), 1U);
    EXPECT_EQ (string_distance ("abc"s, ""s), 3U);
    EXPECT_EQ (string_distance (""s, "abc"s), 3U);
}

TEST (StringDistance, EqualStrings) {
    EXPECT_EQ (string_distance (""s, ""s), 0U);
    EXPECT_EQ (string_distance ("a"s, "a"s), 0U);
    EXPECT_EQ (string_distance ("abc"s, "abc"s), 0U);
}

TEST (StringDistance, InsertsOnly) {
    EXPECT_EQ (string_distance (""s, "a"s), 1U);
    EXPECT_EQ (string_distance ("a"s, "ab"s), 1U);
    EXPECT_EQ (string_distance ("b"s, "ab"s), 1U);
    EXPECT_EQ (string_distance ("ac"s, "abc"s), 1U);
    EXPECT_EQ (string_distance ("abcdefg"s, "xabxcdxxefxgx"s), 6U);
}

TEST (StringDistance, DeletesOnly) {
    EXPECT_EQ (string_distance ("a"s, ""s), 1U);
    EXPECT_EQ (string_distance ("ab"s, "a"s), 1U);
    EXPECT_EQ (string_distance ("ab"s, "b"s), 1U);
    EXPECT_EQ (string_distance ("abc"s, "ac"s), 1U);
    EXPECT_EQ (string_distance ("xabxcdxxefxgx"s, "abcdefg"s), 6U);
}

TEST (StringDistance, SubstitutionsOnly) {
    EXPECT_EQ (string_distance ("a"s, "b"s), 1U);
    EXPECT_EQ (string_distance ("ab"s, "ac"s), 1U);
    EXPECT_EQ (string_distance ("ac"s, "bc"s), 1U);
    EXPECT_EQ (string_distance ("abc"s, "axc"s), 1U);
    EXPECT_EQ (string_distance ("xabxcdxxefxgx"s, "1ab2cd34ef5g6"s), 6U);
}

TEST (StringDistance, VariedOperations) {
    EXPECT_EQ (string_distance ("example"s, "samples"s), 3U);
    EXPECT_EQ (string_distance ("sturgeon"s, "urgently"s), 6U);
    EXPECT_EQ (string_distance ("levenshtein"s, "frankenstein"s), 6U);
    EXPECT_EQ (string_distance ("distance"s, "difference"s), 5U);
}
