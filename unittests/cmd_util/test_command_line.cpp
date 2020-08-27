//*                                                _   _ _             *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | | (_)_ __   ___  *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | | | | '_ \ / _ \ *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | | | | | | |  __/ *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| |_|_|_| |_|\___| *
//*                                                                    *
//===- unittests/cmd_util/test_command_line.cpp ---------------------------===//
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
#include "pstore/cmd_util/command_line.hpp"

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
#if defined(_WIN32) && defined(_UNICODE)
        using string_stream = std::wostringstream;
#else
        using string_stream = std::ostringstream;
#endif

        void add () {}
        template <typename... Strs>
        void add (std::string const & s, Strs... strs) {
            strings_.push_back (s);
            this->add (strs...);
        }

        bool parse_command_line_options (string_stream & output, string_stream & errors) {
            return cl::details::parse_command_line_options (
                std::begin (strings_), std::end (strings_), "overview", output, errors);
        }

    private:
        std::list<std::string> strings_;
    };

    ClCommandLine::ClCommandLine () { cl::option::reset_container (); }
    ClCommandLine::~ClCommandLine () {
        cl::option::reset_container ();
        strings_.clear ();
    }

} // end anonymous namespace


TEST_F (ClCommandLine, SingleLetterStringOption) {
    cl::opt<std::string> option ("S");
    this->add ("progname", "-Svalue");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (option.get (), "value");
    EXPECT_EQ (option.get_num_occurrences (), 1U);
}

TEST_F (ClCommandLine, SingleLetterStringOptionSeparateValue) {
    cl::opt<std::string> option ("S");
    this->add ("progname", "-S", "value");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (option.get (), "value");
    EXPECT_EQ (option.get_num_occurrences (), 1U);
}

TEST_F (ClCommandLine, BooleanOption) {
    cl::opt<bool> option ("arg");
    EXPECT_EQ (option.get (), false);

    this->add ("progname", "--arg");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (option.get (), true);
    EXPECT_EQ (option.get_num_occurrences (), 1U);
}

TEST_F (ClCommandLine, SingleLetterBooleanOptions) {
    cl::opt<bool> opt_a ("a");
    cl::opt<bool> opt_b ("b");
    cl::opt<bool> opt_c ("c");
    EXPECT_EQ (opt_a.get (), false);
    EXPECT_EQ (opt_b.get (), false);
    EXPECT_EQ (opt_c.get (), false);

    this->add ("progname", "-ab");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);
    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (opt_a.get (), true);
    EXPECT_EQ (opt_a.get_num_occurrences (), 1U);

    EXPECT_EQ (opt_b.get (), true);
    EXPECT_EQ (opt_b.get_num_occurrences (), 1U);

    EXPECT_EQ (opt_c.get (), false);
    EXPECT_EQ (opt_c.get_num_occurrences (), 0U);
}

TEST_F (ClCommandLine, DoubleDashStringOption) {
    cl::opt<std::string> option ("arg");
    this->add ("progname", "--arg", "value");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);
    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (option.get (), "value");
    EXPECT_EQ (option.get_num_occurrences (), 1U);
}

TEST_F (ClCommandLine, DoubleDashStringOptionWithSingleDash) {
    cl::opt<bool> option ("arg");
    this->add ("progname", "-arg");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_FALSE (res);
    EXPECT_THAT (errors.str (), testing::HasSubstr (NATIVE_TEXT ("Unknown command line argument")));
    EXPECT_THAT (errors.str (), testing::HasSubstr (NATIVE_TEXT ("'--arg'")));
    EXPECT_EQ (output.str ().length (), 0U);
}

TEST_F (ClCommandLine, StringOptionEquals) {
    cl::opt<std::string> option ("arg");
    this->add ("progname", "--arg=value");

    string_stream output;
    string_stream errors;
    EXPECT_TRUE (this->parse_command_line_options (output, errors));

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (option.get (), "value");
    EXPECT_EQ (option.get_num_occurrences (), 1U);
}

TEST_F (ClCommandLine, UnknownArgument) {
    this->add ("progname", "--arg");

    string_stream output;
    string_stream errors;
    EXPECT_FALSE (this->parse_command_line_options (output, errors));
    EXPECT_THAT (errors.str (), testing::HasSubstr (NATIVE_TEXT ("Unknown command line argument")));
    EXPECT_EQ (output.str ().length (), 0U);
}

TEST_F (ClCommandLine, NearestName) {
    cl::opt<std::string> option1 ("aa");
    cl::opt<std::string> option2 ("xx");
    cl::opt<std::string> option3 ("yy");
    this->add ("progname", "--xxx=value");

    string_stream output;
    string_stream errors;
    EXPECT_FALSE (this->parse_command_line_options (output, errors));
    EXPECT_THAT (errors.str (), testing::HasSubstr (NATIVE_TEXT ("Did you mean '--xx=value'")));
    EXPECT_EQ (output.str ().length (), 0U);
}

TEST_F (ClCommandLine, MissingOptionName) {
    this->add ("progname", "--=a");

    string_stream output;
    string_stream errors;
    EXPECT_FALSE (this->parse_command_line_options (output, errors));
    EXPECT_THAT (errors.str (), testing::HasSubstr (NATIVE_TEXT ("Unknown command line argument")));
    EXPECT_EQ (output.str ().length (), 0U);
}

TEST_F (ClCommandLine, StringPositional) {
    cl::opt<std::string> option ("arg", cl::positional);
    EXPECT_EQ (option.get (), "") << "Expected inital string value to be empty";

    this->add ("progname", "hello");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (option.get (), "hello");
}

TEST_F (ClCommandLine, RequiredStringPositional) {
    cl::opt<std::string> option ("arg", cl::positional, cl::required);

    this->add ("progname");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_FALSE (res);

    EXPECT_THAT (errors.str (),
                 testing::HasSubstr (NATIVE_TEXT ("a positional argument was missing")));
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (option.get (), "");
}

TEST_F (ClCommandLine, TwoPositionals) {
    cl::opt<std::string> opt1 ("opt1", cl::positional);
    cl::opt<std::string> opt2 ("opt2", cl::positional);

    this->add ("progname", "arg1", "arg2");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (opt1.get (), "arg1");
    EXPECT_EQ (opt2.get (), "arg2");
}

TEST_F (ClCommandLine, List) {
    cl::list<std::string> opt{"opt"};

    this->add ("progname", "--opt", "foo", "--opt", "bar");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_THAT (opt, ::testing::ElementsAre ("foo", "bar"));
}

namespace {

    enum class enumeration { a, b, c };

    std::ostream & operator<< (std::ostream & os, enumeration e) {
        auto str = "";
        switch (e) {
        case enumeration::a: str = "a"; break;
        case enumeration::b: str = "b"; break;
        case enumeration::c: str = "c"; break;
        }
        return os << str;
    }

} // namespace

TEST_F (ClCommandLine, ListOfEnums) {
    cl::list<enumeration> opt{
        "opt", cl::values (cl::literal{"a", static_cast<int> (enumeration::a), "a description"},
                           cl::literal{"b", static_cast<int> (enumeration::b), "b description"},
                           cl::literal{"c", static_cast<int> (enumeration::c), "c description"})};
    this->add ("progname", "--opt", "a", "--opt", "b");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_THAT (opt, ::testing::ElementsAre (enumeration::a, enumeration::b));
}

TEST_F (ClCommandLine, ListSingleDash) {
    cl::list<std::string> opt{"o"};

    this->add ("progname", "-oa", "-o", "b", "-oc");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_THAT (opt, testing::ElementsAre ("a", "b", "c"));
}

TEST_F (ClCommandLine, ListPositional) {
    cl::list<std::string> opt ("opt", cl::positional);

    this->add ("progname", "foo", "bar");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_THAT (opt, ::testing::ElementsAre ("foo", "bar"));
}

TEST_F (ClCommandLine, ListCsvEnabled) {
    cl::list<std::string> opt ("opt", cl::positional, cl::comma_separated);

    this->add ("progname", "a,b", "c,d");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_THAT (opt, ::testing::ElementsAre ("a", "b", "c", "d"));
}

TEST_F (ClCommandLine, ListCsvDisabled) {
    cl::list<std::string> opt ("opt", cl::positional);

    this->add ("progname", "a,b");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_THAT (opt, ::testing::ElementsAre ("a,b"));
}


TEST_F (ClCommandLine, MissingRequired) {
    cl::opt<std::string> opt ("opt", cl::required);

    this->add ("progname");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_FALSE (res);

    EXPECT_THAT (errors.str (),
                 testing::HasSubstr (NATIVE_TEXT ("must be specified at least once")));
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (opt.get_num_occurrences (), 0U);
    EXPECT_EQ (opt.get (), "");
}

TEST_F (ClCommandLine, MissingValue) {
    cl::opt<std::string> opt{"opt", cl::required};

    this->add ("progname", "--opt");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_FALSE (res);

    EXPECT_THAT (errors.str (), testing::HasSubstr (NATIVE_TEXT ("requires a value")));
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (opt.get (), "");
}

TEST_F (ClCommandLine, UnwantedValue) {
    cl::opt<bool> opt{"opt"};

    this->add ("progname", "--opt=true");

    string_stream output;
    string_stream errors;
    EXPECT_FALSE (this->parse_command_line_options (output, errors));
    EXPECT_THAT (errors.str (), ::testing::HasSubstr (NATIVE_TEXT ("does not take a value")));
    EXPECT_EQ (output.str ().length (), 0U);
    EXPECT_FALSE (opt.get ());
}

TEST_F (ClCommandLine, DoubleDashSwitchToPositional) {
    cl::opt<std::string> opt ("opt");
    cl::list<std::string> positional ("names", cl::positional);

    this->add ("progname", "--", "-opt", "foo");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (opt.get_num_occurrences (), 0U);
    EXPECT_EQ (opt.get (), "");
    EXPECT_THAT (positional, ::testing::ElementsAre ("-opt", "foo"));
}

TEST_F (ClCommandLine, AliasBool) {
    cl::opt<bool> opt ("opt");
    cl::alias opt2 ("o", cl::aliasopt (opt));

    this->add ("progname", "-o");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);
    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (opt.get_num_occurrences (), 1U);
    EXPECT_EQ (opt.get (), true);
    EXPECT_EQ (opt2.get_num_occurrences (), 1U);
}
