//===- unittests/broker/test_spawn.cpp ------------------------------------===//
//*                                  *
//*  ___ _ __   __ ___      ___ __   *
//* / __| '_ \ / _` \ \ /\ / / '_ \  *
//* \__ \ |_) | (_| |\ V  V /| | | | *
//* |___/ .__/ \__,_| \_/\_/ |_| |_| *
//*     |_|                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/broker/spawn.hpp"

#include <initializer_list>
#include <iterator>
#include <vector>

#include "pstore/support/portab.hpp"
#include "gmock/gmock.h"

using namespace pstore::broker;

#ifdef _WIN32

TEST (Win32ArgvQuote, Empty) {
    EXPECT_EQ ("\"\"", win32::argv_quote ("", false));
    EXPECT_EQ ("\"\"", win32::argv_quote ("", true));
}
TEST (Win32ArgvQuote, Trivial) {
    EXPECT_EQ ("abc", win32::argv_quote ("abc"));
    EXPECT_EQ ("\"abc\"", win32::argv_quote ("abc", true));
}
TEST (Win32ArgvQuote, SingleSpecialCharacter) {
    EXPECT_EQ ("\"a bc\"", win32::argv_quote ("a bc", false));
    EXPECT_EQ ("\"a bc\"", win32::argv_quote ("a bc", true));
    EXPECT_EQ ("\"a\tbc\"", win32::argv_quote ("a\tbc"));
    EXPECT_EQ ("\"a\tbc\"", win32::argv_quote ("a\tbc"));
    EXPECT_EQ ("\"a\nbc\"", win32::argv_quote ("a\nbc"));
}

#    define BS "\\"    // backslash
#    define QUOTE "\"" // double quote

TEST (Win32ArgvQuote, SingleBackslash) {
    EXPECT_EQ (QUOTE "a" BS "bc" QUOTE, win32::argv_quote ("a\\bc", true));
    EXPECT_EQ (QUOTE "abc" BS BS QUOTE, win32::argv_quote ("abc\\", true));
    EXPECT_EQ (QUOTE "abc" BS BS BS QUOTE QUOTE, win32::argv_quote ("abc" BS QUOTE));
}

// Examples taken from the MSDN article "Parsing C++ Command-Line Arguments"
TEST (Win32ArgvQuote, SingleQuote) {
    EXPECT_EQ (QUOTE "a" BS BS BS "b" QUOTE, win32::argv_quote ("a" BS BS BS "b"));
    EXPECT_EQ (QUOTE "a" BS BS BS QUOTE "b" QUOTE, win32::argv_quote ("a" BS QUOTE "b"));
    EXPECT_EQ (QUOTE "a" BS BS "b c" QUOTE, win32::argv_quote ("a" BS BS "b c"));
    EXPECT_EQ (QUOTE "de fg" QUOTE, win32::argv_quote ("de fg"));
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
        return win32::build_command_line (v.data ());
    }

} // namespace

TEST_F (Win32CommandLine, Simple) {
    EXPECT_EQ ("abc d e", build ({"abc", "d", "e"}));
    EXPECT_EQ ("ab \"de fg\" h", build ({"ab", "de fg", "h"}));
}

#endif // _WIN32
