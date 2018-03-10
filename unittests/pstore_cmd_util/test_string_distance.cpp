//*      _        _                   _ _     _                        *
//*  ___| |_ _ __(_)_ __   __ _    __| (_)___| |_ __ _ _ __   ___ ___  *
//* / __| __| '__| | '_ \ / _` |  / _` | / __| __/ _` | '_ \ / __/ _ \ *
//* \__ \ |_| |  | | | | | (_| | | (_| | \__ \ || (_| | | | | (_|  __/ *
//* |___/\__|_|  |_|_| |_|\__, |  \__,_|_|___/\__\__,_|_| |_|\___\___| *
//*                       |___/                                        *
//===- unittests/pstore_cmd_util/test_string_distance.cpp -----------------===//
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
#include "pstore/cmd_util/cl/string_distance.hpp"

#include "gtest/gtest.h"

using pstore::cmd_util::cl::string_distance;

TEST (StringDistance, EmptyStrings) {
    EXPECT_EQ (string_distance (std::string{""}, std::string{""}), 0U);
    EXPECT_EQ (string_distance (std::string{"a"}, std::string{""}), 1U);
    EXPECT_EQ (string_distance (std::string{""}, std::string{"a"}), 1U);
    EXPECT_EQ (string_distance (std::string{"abc"}, std::string{""}), 3U);
    EXPECT_EQ (string_distance (std::string{""}, std::string{"abc"}), 3U);
}

TEST (StringDistance, EqualStrings) {
    EXPECT_EQ (string_distance (std::string{""}, std::string{""}), 0U);
    EXPECT_EQ (string_distance (std::string{"a"}, std::string{"a"}), 0U);
    EXPECT_EQ (string_distance (std::string{"abc"}, std::string{"abc"}), 0U);
}

TEST (StringDistance, InsertsOnly) {
    EXPECT_EQ (string_distance (std::string{""}, std::string{"a"}), 1U);
    EXPECT_EQ (string_distance (std::string{"a"}, std::string{"ab"}), 1U);
    EXPECT_EQ (string_distance (std::string{"b"}, std::string{"ab"}), 1U);
    EXPECT_EQ (string_distance (std::string{"ac"}, std::string{"abc"}), 1U);
    EXPECT_EQ (string_distance (std::string{"abcdefg"}, std::string{"xabxcdxxefxgx"}), 6U);
}

TEST (StringDistance, DeletesOnly) {
    EXPECT_EQ (string_distance (std::string{"a"}, std::string{""}), 1U);
    EXPECT_EQ (string_distance (std::string{"ab"}, std::string{"a"}), 1U);
    EXPECT_EQ (string_distance (std::string{"ab"}, std::string{"b"}), 1U);
    EXPECT_EQ (string_distance (std::string{"abc"}, std::string{"ac"}), 1U);
    EXPECT_EQ (string_distance (std::string{"xabxcdxxefxgx"}, std::string{"abcdefg"}), 6U);
}

TEST (StringDistance, SubstitutionsOnly) {
    EXPECT_EQ (string_distance (std::string{"a"}, std::string{"b"}), 1U);
    EXPECT_EQ (string_distance (std::string{"ab"}, std::string{"ac"}), 1U);
    EXPECT_EQ (string_distance (std::string{"ac"}, std::string{"bc"}), 1U);
    EXPECT_EQ (string_distance (std::string{"abc"}, std::string{"axc"}), 1U);
    EXPECT_EQ (string_distance (std::string{"xabxcdxxefxgx"}, std::string{"1ab2cd34ef5g6"}), 6U);
}

TEST (StringDistance, VariedOperations) {
    EXPECT_EQ (string_distance (std::string{"example"}, std::string{"samples"}), 3U);
    EXPECT_EQ (string_distance (std::string{"sturgeon"}, std::string{"urgently"}), 6U);
    EXPECT_EQ (string_distance (std::string{"levenshtein"}, std::string{"frankenstein"}), 6U);
    EXPECT_EQ (string_distance (std::string{"distance"}, std::string{"difference"}), 5U);
}
// eof: unittests/pstore_cmd_util/test_string_distance.cpp
