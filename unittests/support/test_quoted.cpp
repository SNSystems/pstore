//*                    _           _  *
//*   __ _ _   _  ___ | |_ ___  __| | *
//*  / _` | | | |/ _ \| __/ _ \/ _` | *
//* | (_| | |_| | (_) | ||  __/ (_| | *
//*  \__, |\__,_|\___/ \__\___|\__,_| *
//*     |_|                           *
//===- unittests/support/test_quoted.cpp ----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/support/quoted.hpp"

#include <gtest/gtest.h>

using namespace std::string_literals;

TEST (Quoted, Empty) {
    std::stringstream str;
    str << pstore::quoted ("");
    EXPECT_EQ (str.str (), R"("")");
}

TEST (Quoted, Simple) {
    std::stringstream str;
    str << pstore::quoted ("simple");
    EXPECT_EQ (str.str (), R"("simple")");
}

TEST (Quoted, SimpleStdString) {
    std::stringstream str;
    str << pstore::quoted ("simple"s);
    EXPECT_EQ (str.str (), R"("simple")");
}

TEST (Quoted, Backslash) {
    std::stringstream str;
    str << pstore::quoted (R"(one\two)");
    EXPECT_EQ (str.str (), R"("one\two")") << "Backslash characters should not be escaped";
}
