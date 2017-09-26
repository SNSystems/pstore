//*                                  *
//*  ___ _ __   __ ___      ___ __   *
//* / __| '_ \ / _` \ \ /\ / / '_ \  *
//* \__ \ |_) | (_| |\ V  V /| | | | *
//* |___/ .__/ \__,_| \_/\_/ |_| |_| *
//*     |_|                          *
//===- unittests/broker/test_spawn.cpp ------------------------------------===//
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
#include "broker/spawn.hpp"

#include <initializer_list>
#include <iterator>
#include <vector>

#include "pstore_support/portab.hpp"
#include "gmock/gmock.h"

#ifdef _WIN32

TEST (Win32ArgvQuote, Empty) {
    EXPECT_EQ ("\"\"", broker::win32::argv_quote ("", false));
    EXPECT_EQ ("\"\"", broker::win32::argv_quote ("", true));
}
TEST (Win32ArgvQuote, Trivial) {
    EXPECT_EQ ("abc", broker::win32::argv_quote ("abc"));
    EXPECT_EQ ("\"abc\"", broker::win32::argv_quote ("abc", true));
}
TEST (Win32ArgvQuote, SingleSpecialCharacter) {
    EXPECT_EQ ("\"a bc\"", broker::win32::argv_quote ("a bc", false));
    EXPECT_EQ ("\"a bc\"", broker::win32::argv_quote ("a bc", true));
    EXPECT_EQ ("\"a\tbc\"", broker::win32::argv_quote ("a\tbc"));
    EXPECT_EQ ("\"a\tbc\"", broker::win32::argv_quote ("a\tbc"));
    EXPECT_EQ ("\"a\nbc\"", broker::win32::argv_quote ("a\nbc"));
}

#define BS "\\"    // backslash
#define QUOTE "\"" // double quote

TEST (Win32ArgvQuote, SingleBackslash) {
    EXPECT_EQ (QUOTE "a" BS "bc" QUOTE, broker::win32::argv_quote ("a\\bc", true));
    EXPECT_EQ (QUOTE "abc" BS BS QUOTE, broker::win32::argv_quote ("abc\\", true));
    EXPECT_EQ (QUOTE "abc" BS BS BS QUOTE QUOTE, broker::win32::argv_quote ("abc" BS QUOTE));
}

// Examples taken from the MSDN article "Parsing C++ Command-Line Arguments"
TEST (Win32ArgvQuote, SingleQuote) {
    EXPECT_EQ (QUOTE "a" BS BS BS "b" QUOTE, broker::win32::argv_quote ("a" BS BS BS "b"));
    EXPECT_EQ (QUOTE "a" BS BS BS QUOTE "b" QUOTE, broker::win32::argv_quote ("a" BS QUOTE "b"));
    EXPECT_EQ (QUOTE "a" BS BS "b c" QUOTE, broker::win32::argv_quote ("a" BS BS "b c"));
    EXPECT_EQ (QUOTE "de fg" QUOTE, broker::win32::argv_quote ("de fg"));
}


namespace {

    class Win32CommandLine : public ::testing::Test {
    protected:
        std::string build (std::initializer_list<char const *> args);
    };

    std::string Win32CommandLine::build (std::initializer_list<char const *> args) {
        std::vector<char const *> v;
        std::copy (std::begin (args), std::end (args), std::back_inserter (v));
        v.push_back (nullptr);
        return broker::win32::build_command_line (v.data ());
    }

} // (anonymous namespace)

TEST_F (Win32CommandLine, Simple) {
    EXPECT_EQ ("abc d e", build ({"abc", "d", "e"}));
    EXPECT_EQ ("ab \"de fg\" h", build ({"ab", "de fg", "h"}));
}

#endif // _WIN32
// eof: unittests/broker/test_spawn.cpp
