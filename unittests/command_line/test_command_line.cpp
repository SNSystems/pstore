//===- unittests/command_line/test_command_line.cpp -----------------------===//
//*                                                _   _ _             *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | | (_)_ __   ___  *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | | | | '_ \ / _ \ *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | | | | | | |  __/ *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| |_|_|_| |_|\___| *
//*                                                                    *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/command_line.hpp"

// Standard library includes
#include <algorithm>
#include <cstring>
#include <iterator>
#include <list>
#include <vector>

// #rd party includes
#include <gmock/gmock.h>

using namespace pstore::command_line;

namespace {

    class ClCommandLine : public ::testing::Test {
    public:
        ClCommandLine ();
        ~ClCommandLine () override;

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
            return details::parse_command_line_options (std::begin (strings_), std::end (strings_),
                                                        "overview", output, errors);
        }

    private:
        std::list<std::string> strings_;
    };

    ClCommandLine::ClCommandLine () { option::reset_container (); }
    ClCommandLine::~ClCommandLine () {
        option::reset_container ();
        strings_.clear ();
    }

} // end anonymous namespace


TEST_F (ClCommandLine, SingleLetterStringOption) {
    opt<std::string> option ("S");
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
    opt<std::string> option ("S");
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
    opt<bool> option ("arg");
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
    opt<bool> opt_a ("a");
    opt<bool> opt_b ("b");
    opt<bool> opt_c ("c");
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
    opt<std::string> option ("arg");
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
    opt<bool> option ("arg");
    this->add ("progname", "-arg");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_FALSE (res);
    EXPECT_THAT (errors.str (),
                 testing::HasSubstr (PSTORE_NATIVE_TEXT ("Unknown command line argument")));
    EXPECT_THAT (errors.str (), testing::HasSubstr (PSTORE_NATIVE_TEXT ("'--arg'")));
    EXPECT_EQ (output.str ().length (), 0U);
}

TEST_F (ClCommandLine, StringOptionEquals) {
    opt<std::string> option ("arg");
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
    EXPECT_THAT (errors.str (),
                 testing::HasSubstr (PSTORE_NATIVE_TEXT ("Unknown command line argument")));
    EXPECT_EQ (output.str ().length (), 0U);
}

TEST_F (ClCommandLine, NearestName) {
    opt<std::string> option1 ("aa");
    opt<std::string> option2 ("xx");
    opt<std::string> option3 ("yy");
    this->add ("progname", "--xxx=value");

    string_stream output;
    string_stream errors;
    EXPECT_FALSE (this->parse_command_line_options (output, errors));
    EXPECT_THAT (errors.str (),
                 testing::HasSubstr (PSTORE_NATIVE_TEXT ("Did you mean '--xx=value'")));
    EXPECT_EQ (output.str ().length (), 0U);
}

TEST_F (ClCommandLine, MissingOptionName) {
    this->add ("progname", "--=a");

    string_stream output;
    string_stream errors;
    EXPECT_FALSE (this->parse_command_line_options (output, errors));
    EXPECT_THAT (errors.str (),
                 testing::HasSubstr (PSTORE_NATIVE_TEXT ("Unknown command line argument")));
    EXPECT_EQ (output.str ().length (), 0U);
}

TEST_F (ClCommandLine, StringPositional) {
    opt<std::string> option ("arg", positional);
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
    opt<std::string> option ("arg", positional, required);

    this->add ("progname");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_FALSE (res);

    EXPECT_THAT (errors.str (),
                 testing::HasSubstr (PSTORE_NATIVE_TEXT ("a positional argument was missing")));
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (option.get (), "");
}

TEST_F (ClCommandLine, TwoPositionals) {
    opt<std::string> opt1 ("opt1", positional);
    opt<std::string> opt2 ("opt2", positional);

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
    list<std::string> opt{"opt"};

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
    list<enumeration> opt{
        "opt", values (literal{"a", static_cast<int> (enumeration::a), "a description"},
                       literal{"b", static_cast<int> (enumeration::b), "b description"},
                       literal{"c", static_cast<int> (enumeration::c), "c description"})};
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
    list<std::string> opt{"o"};

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
    list<std::string> opt ("opt", positional);

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
    list<std::string> opt ("opt", positional, comma_separated);

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
    list<std::string> opt{"opt", positional};

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
    opt<std::string> opt{"opt", required};

    this->add ("progname");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_FALSE (res);

    EXPECT_THAT (errors.str (),
                 testing::HasSubstr (PSTORE_NATIVE_TEXT ("must be specified at least once")));
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (opt.get_num_occurrences (), 0U);
    EXPECT_EQ (opt.get (), "");
}

TEST_F (ClCommandLine, MissingValue) {
    opt<std::string> opt{"opt", required};

    this->add ("progname", "--opt");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_FALSE (res);

    EXPECT_THAT (errors.str (), testing::HasSubstr (PSTORE_NATIVE_TEXT ("requires a value")));
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (opt.get (), "");
}

TEST_F (ClCommandLine, UnwantedValue) {
    opt<bool> opt{"opt"};

    this->add ("progname", "--opt=true");

    string_stream output;
    string_stream errors;
    EXPECT_FALSE (this->parse_command_line_options (output, errors));
    EXPECT_THAT (errors.str (),
                 ::testing::HasSubstr (PSTORE_NATIVE_TEXT ("does not take a value")));
    EXPECT_EQ (output.str ().length (), 0U);
    EXPECT_FALSE (opt.get ());
}

TEST_F (ClCommandLine, DoubleDashSwitchToPositional) {
    opt<std::string> opt{"opt"};
    list<std::string> p{"names", positional};

    this->add ("progname", "--", "-opt", "foo");

    string_stream output;
    string_stream errors;
    bool const res = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (opt.get_num_occurrences (), 0U);
    EXPECT_EQ (opt.get (), "");
    EXPECT_THAT (p, ::testing::ElementsAre ("-opt", "foo"));
}

TEST_F (ClCommandLine, AliasBool) {
    opt<bool> opt{"opt"};
    alias opt2{"o", aliasopt{opt}};

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

TEST_F (ClCommandLine, TwoCallsToParser) {
    opt<std::string> option ("S");
    this->add ("progname", "-Svalue");

    string_stream output;
    string_stream errors;
    bool const res1 = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res1);
    bool const res2 = this->parse_command_line_options (output, errors);
    EXPECT_TRUE (res2);

    EXPECT_EQ (errors.str ().length (), 0U);
    EXPECT_EQ (output.str ().length (), 0U);

    EXPECT_EQ (option.get (), "value");
    // We saw the -S switch twice because the arguments were parsed twice.
    EXPECT_EQ (option.get_num_occurrences (), 2U);
}
