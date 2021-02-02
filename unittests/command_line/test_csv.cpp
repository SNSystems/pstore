//*                  *
//*   ___ _____   __ *
//*  / __/ __\ \ / / *
//* | (__\__ \\ V /  *
//*  \___|___/ \_/   *
//*                  *
//===- unittests/command_line/test_csv.cpp --------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/csv.hpp"

#include <gmock/gmock.h>

using pstore::command_line::csv;
using testing::ElementsAre;

TEST (Csv, Empty) {
    EXPECT_THAT (csv (""), ElementsAre (""));
}

TEST (Csv, SimpleValues) {
    EXPECT_THAT (csv ("a"), ElementsAre ("a"));
    EXPECT_THAT (csv ("a,b"), ElementsAre ("a", "b"));
    EXPECT_THAT (csv ("a,b,c"), ElementsAre ("a", "b", "c"));
}

TEST (Csv, TrailingComma) {
    EXPECT_THAT (csv ("a,"), ElementsAre ("a", ""));
}

TEST (Csv, LeadingComma) {
    EXPECT_THAT (csv (",a"), ElementsAre ("", "a"));
}

TEST (Csv, EmptyContainer) {
    std::list<std::string> l;
    EXPECT_THAT (csv (std::begin (l), std::end (l)), ::testing::IsEmpty ());
}

TEST (Csv, ContainerWithOneMemberTwoValues) {
    std::list<std::string> l{{"a,b"}};
    EXPECT_THAT (csv (std::begin (l), std::end (l)), ElementsAre ("a", "b"));
}
TEST (Csv, ContainerWithTwoMembersFourValues) {
    std::list<std::string> l{{"a,b"}, {"c,d"}};
    EXPECT_THAT (csv (std::begin (l), std::end (l)), ElementsAre ("a", "b", "c", "d"));
}
