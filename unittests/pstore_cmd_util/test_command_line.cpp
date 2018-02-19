//*                                                _   _ _             *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | | (_)_ __   ___  *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | | | | '_ \ / _ \ *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | | | | | | |  __/ *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| |_|_|_| |_|\___| *
//*                                                                    *
//===- unittests/pstore_cmd_util/test_command_line.cpp --------------------===//
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
#include "pstore_cmd_util/cl/command_line.hpp"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <list>
#include <vector>

#include "gmock/gmock.h"
using namespace pstore::cmd_util;

namespace {

    class ClCommandLine : public ::testing::Test {
    public:
        ClCommandLine ();
        ~ClCommandLine ();

    protected:
        void add () {}
        template <typename... Strs>
        void add (std::string const & s, Strs... strs) {
            strings_.push_back (s);
            this->add (strs...);
        }

        bool parse_command_line_options (std::ostream & errors) {
            return cl::details::ParseCommandLineOptions (std::begin (strings_), std::end (strings_),
                                                         "overview", &errors);
        }

    private:
        std::list<std::string> strings_;
    };

    ClCommandLine::ClCommandLine () { cl::option::reset_container (); }
    ClCommandLine::~ClCommandLine () {
        cl::option::reset_container ();
        strings_.clear ();
    }

} // namespace


TEST_F (ClCommandLine, StringOption) {
    cl::opt<std::string> option ("arg");
    this->add ("progname", "-arg", "hello");

    std::ostringstream errors;
    bool res = this->parse_command_line_options (errors);
    EXPECT_TRUE (res);
    EXPECT_EQ (errors.str (), "");
    EXPECT_EQ (std::string{option}, "hello");
}

TEST_F (ClCommandLine, StringOptionEquals) {
    cl::opt<std::string> option ("arg");
    this->add ("progname", "-arg=hello");
    std::ostringstream errors;
    bool res = this->parse_command_line_options (errors);
    EXPECT_TRUE (res);
    EXPECT_EQ (errors.str (), "");
    EXPECT_EQ (std::string{option}, "hello");
}

TEST_F (ClCommandLine, StringPositional) {
    cl::opt<std::string> option ("arg", cl::Positional);
    EXPECT_EQ (std::string{option}, "") << "Expected inital string value to be empty";

    this->add ("progname", "hello");
    std::ostringstream errors;
    bool res = this->parse_command_line_options (errors);
    EXPECT_TRUE (res);
    EXPECT_EQ (errors.str (), "");
    EXPECT_EQ (std::string{option}, "hello");
}

TEST_F (ClCommandLine, RequiredStringPositional) {
    cl::opt<std::string> option ("arg", cl::Positional, cl::Required);

    this->add ("progname");
    std::ostringstream errors;
    bool res = this->parse_command_line_options (errors);
    EXPECT_FALSE (res);
    EXPECT_THAT (errors.str (), ::testing::HasSubstr ("a positional argument was missing"));
    EXPECT_EQ (std::string{option}, "");
}

TEST_F (ClCommandLine, TwoPositionals) {
    cl::opt<std::string> opt1 ("opt1", cl::Positional);
    cl::opt<std::string> opt2 ("opt2", cl::Positional);

    this->add ("progname", "arg1", "arg2");
    std::ostringstream errors;
    bool res = this->parse_command_line_options (errors);
    EXPECT_TRUE (res);
    EXPECT_EQ (errors.str (), "");
    EXPECT_EQ (std::string{opt1}, "arg1");
    EXPECT_EQ (std::string{opt2}, "arg2");
}

TEST_F (ClCommandLine, List) {
    cl::list<std::string> opt ("opt");

    this->add ("progname", "-opt", "foo", "-opt", "bar");
    std::ostringstream errors;
    bool res = this->parse_command_line_options (errors);
    EXPECT_TRUE (res);
    EXPECT_EQ (errors.str (), "");
    EXPECT_THAT (opt, ::testing::ElementsAre ("foo", "bar"));
}

TEST_F (ClCommandLine, ListPositional) {
    cl::list<std::string> opt ("opt", cl::Positional);

    this->add ("progname", "foo", "bar");
    std::ostringstream errors;
    bool res = this->parse_command_line_options (errors);
    EXPECT_TRUE (res);
    EXPECT_EQ (errors.str (), "");
    EXPECT_THAT (opt, ::testing::ElementsAre ("foo", "bar"));
}

TEST_F (ClCommandLine, MissingRequired) {
    cl::opt<std::string> opt ("opt", cl::Required);

    this->add ("progname");
    std::ostringstream errors;
    bool res = this->parse_command_line_options (errors);
    EXPECT_FALSE (res);
    EXPECT_THAT (errors.str (), ::testing::HasSubstr ("must be specified at least once"));
    EXPECT_EQ (opt.getNumOccurrences (), 0U);
    EXPECT_EQ (std::string{opt}, "");
}

TEST_F (ClCommandLine, MissingValue) {
    cl::opt<std::string> opt ("opt", cl::Required);

    this->add ("progname", "-opt");
    std::ostringstream errors;
    bool res = this->parse_command_line_options (errors);
    EXPECT_FALSE (res);
    EXPECT_THAT (errors.str (), ::testing::HasSubstr ("requires a value"));
    EXPECT_EQ (std::string{opt}, "");
}

TEST_F (ClCommandLine, DoubleDashSwitchToPositional) {
    cl::opt<std::string> opt ("opt");
    cl::list<std::string> positional ("names", cl::Positional);

    this->add ("progname", "--", "-opt", "foo");
    std::ostringstream errors;
    bool res = this->parse_command_line_options (errors);
    EXPECT_TRUE (res);
    EXPECT_EQ (opt.getNumOccurrences (), 0U);
    EXPECT_EQ (std::string{opt}, "");
    EXPECT_THAT (positional, ::testing::ElementsAre ("-opt", "foo"));
}

// eof: unittests/pstore_cmd_util/test_command_line.cpp
