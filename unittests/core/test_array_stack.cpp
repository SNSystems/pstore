//*                                   _             _     *
//*   __ _ _ __ _ __ __ _ _   _   ___| |_ __ _  ___| | __ *
//*  / _` | '__| '__/ _` | | | | / __| __/ _` |/ __| |/ / *
//* | (_| | |  | | | (_| | |_| | \__ \ || (_| | (__|   <  *
//*  \__,_|_|  |_|  \__,_|\__, | |___/\__\__,_|\___|_|\_\ *
//*                       |___/                           *
//===- unittests/core/test_array_stack.cpp --------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
