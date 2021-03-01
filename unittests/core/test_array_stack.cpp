//===- unittests/core/test_array_stack.cpp --------------------------------===//
//*                                   _             _     *
//*   __ _ _ __ _ __ __ _ _   _   ___| |_ __ _  ___| | __ *
//*  / _` | '__| '__/ _` | | | | / __| __/ _` |/ __| |/ / *
//* | (_| | |  | | | (_| | |_| | \__ \ || (_| | (__|   <  *
//*  \__,_|_|  |_|  \__,_|\__, | |___/\__\__,_|\___|_|\_\ *
//*                       |___/                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/core/array_stack.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include <gtest/gtest.h>

TEST (ArrayStack, Empty) {
    pstore::array_stack<int, 2> stack;
    EXPECT_EQ (0U, stack.size ());
    EXPECT_TRUE (stack.empty ());
}

TEST (ArrayStack, MaxSize) {
    pstore::array_stack<int, 2> stack;
    EXPECT_EQ (2U, stack.max_size ());
}

TEST (ArrayStack, Push1Value) {
    pstore::array_stack<int, 2> stack;
    stack.push (17);
    EXPECT_EQ (1U, stack.size ());
    EXPECT_FALSE (stack.empty ());
    EXPECT_EQ (17, stack.top ());
}

TEST (ArrayStack, PushMoveValue) {
    pstore::array_stack<std::string, 2> stack;

    std::string value{"Hello"};
    stack.push (std::move (value));

    std::string const & top = stack.top ();
    EXPECT_STREQ ("Hello", top.c_str ());
}

TEST (ArrayStack, PushAndPop1Value) {
    pstore::array_stack<int, 2> stack;
    stack.push (31);
    stack.pop ();
    EXPECT_EQ (0U, stack.size ());
    EXPECT_TRUE (stack.empty ());
}

TEST (ArrayStack, PushAndPopMaxValues) {
    pstore::array_stack<int, 2> stack;
    stack.push (31);
    stack.push (33);
    stack.pop ();
    stack.pop ();
    EXPECT_EQ (0U, stack.size ());
    EXPECT_TRUE (stack.empty ());
}
