//*       _                                   *
//*   ___| |  _ __   __ _ _ __ ___  ___ _ __  *
//*  / __| | | '_ \ / _` | '__/ __|/ _ \ '__| *
//* | (__| | | |_) | (_| | |  \__ \  __/ |    *
//*  \___|_| | .__/ \__,_|_|  |___/\___|_|    *
//*          |_|                              *
//===- unittests/pstore_cmd_util/test_cl_parser.cpp -----------------------===//
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
#include "pstore_cmd_util/cl/command_line.hpp"
#include "gtest/gtest.h"

TEST (ClParser, SimpleString) {
    using pstore::cmd_util::cl::maybe;
    using pstore::cmd_util::cl::parser;

    maybe<std::string> r = parser<std::string> () ("hello");
    EXPECT_TRUE (r.has_value ());
    EXPECT_EQ (r.value (), "hello");
}

TEST (ClParser, StringFromSet) {
    using pstore::cmd_util::cl::maybe;
    using pstore::cmd_util::cl::parser;

    parser<std::string> p;
    p.add_literal_option ("a", 31, "description a");
    p.add_literal_option ("b", 37, "description b");

    {
        maybe<std::string> r1 = p ("hello");
        EXPECT_FALSE (r1.has_value ());
    }
    {
        maybe<std::string> r2 = p ("a");
        EXPECT_TRUE (r2.has_value ());
        EXPECT_EQ (r2.value (), "a");
    }
    {
        maybe<std::string> r3 = p ("b");
        EXPECT_TRUE (r3.has_value ());
        EXPECT_EQ (r3.value (), "b");
    }
}

TEST (ClParser, Int) {
    using pstore::cmd_util::cl::maybe;
    using pstore::cmd_util::cl::parser;
    {
        maybe<int> r1 = parser<int> () ("43");
        EXPECT_TRUE (r1.has_value ());
        EXPECT_EQ (r1.value (), 43);
    }
    {
        parser<int> p;
        maybe<int> r2 = p ("");
        EXPECT_FALSE (r2.has_value ());
    }
    {
        maybe<int> r3 = parser<int> () ("bad");
        EXPECT_FALSE (r3.has_value ());
    }
    {
        maybe<int> r4 = parser<int> () ("42bad");
        EXPECT_FALSE (r4.has_value ());
    }
}

TEST (ClParser, Enum) {
    using pstore::cmd_util::cl::maybe;
    using pstore::cmd_util::cl::parser;

    enum color { red, blue, green };
    parser<color> p;
    p.add_literal_option ("red", red, "description red");
    p.add_literal_option ("blue", blue, "description blue");
    p.add_literal_option ("green", green, "description green");
    {
        maybe<color> r1 = p ("red");
        EXPECT_TRUE (r1.has_value ());
        EXPECT_EQ (r1.value (), red);
    }
    {
        maybe<color> r2 = p ("blue");
        EXPECT_TRUE (r2.has_value ());
        EXPECT_EQ (r2.value (), blue);
    }
    {
        maybe<color> r3 = p ("green");
        EXPECT_TRUE (r3.has_value ());
        EXPECT_EQ (r3.value (), green);
    }
    {
        maybe<color> r4 = p ("bad");
        EXPECT_FALSE (r4.has_value ());
    }
    {
        maybe<color> r5 = p ("");
        EXPECT_FALSE (r5.has_value ());
    }
}

TEST (ClParser, Modifiers) {
    using namespace pstore::cmd_util;
    EXPECT_EQ (cl::opt<int> ().get_num_occurrences (), cl::num_occurrences::optional);
    EXPECT_EQ (cl::opt<int>{cl::Optional}.get_num_occurrences (), cl::num_occurrences::optional);
    EXPECT_EQ (cl::opt<int>{cl::Required}.get_num_occurrences (), cl::num_occurrences::required);
    EXPECT_EQ (cl::opt<int>{cl::OneOrMore}.get_num_occurrences (),
               cl::num_occurrences::zero_or_more);
    EXPECT_EQ (cl::opt<int> (cl::Required, cl::OneOrMore).get_num_occurrences (),
               cl::num_occurrences::one_or_more);
    EXPECT_EQ (cl::opt<int> (cl::Optional, cl::OneOrMore).get_num_occurrences (),
               cl::num_occurrences::zero_or_more);

    EXPECT_EQ (cl::opt<int> ().name (), "");
    EXPECT_EQ (cl::opt<int>{"name"}.name (), "name");

    EXPECT_EQ (cl::opt<int>{}.description (), "");
    EXPECT_EQ (cl::opt<int>{cl::desc ("description")}.description (), "description");
}

// eof: unittests/pstore_cmd_util/test_cl_parser.cpp
