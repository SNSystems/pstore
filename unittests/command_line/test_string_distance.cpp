//===- unittests/command_line/test_string_distance.cpp --------------------===//
//*      _        _                   _ _     _                        *
//*  ___| |_ _ __(_)_ __   __ _    __| (_)___| |_ __ _ _ __   ___ ___  *
//* / __| __| '__| | '_ \ / _` |  / _` | / __| __/ _` | '_ \ / __/ _ \ *
//* \__ \ |_| |  | | | | | (_| | | (_| | \__ \ || (_| | | | | (_|  __/ *
//* |___/\__|_|  |_|_| |_|\__, |  \__,_|_|___/\__\__,_|_| |_|\___\___| *
//*                       |___/                                        *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/string_distance.hpp"

// 3rd party includes
#include <gtest/gtest.h>

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
