//===- unittests/command_line/test_help.cpp -------------------------------===//
//*  _          _        *
//* | |__   ___| |_ __   *
//* | '_ \ / _ \ | '_ \  *
//* | | | |  __/ | |_) | *
//* |_| |_|\___|_| .__/  *
//*              |_|     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/help.hpp"

#include <sstream>

#include <gmock/gmock.h>

#include "pstore/command_line/option.hpp"
#include "pstore/command_line/command_line.hpp"

using string_stream = std::ostringstream;

using namespace pstore::command_line;
using namespace std::literals::string_literals;

namespace {

    class Help : public testing::Test {
    public:
        bool parse_command_line_options (string_stream & output, string_stream & errors) {
            std::array<std::string, 2> argv{{"program", "--help"}};
            return details::parse_command_line_options (std::begin (argv), std::end (argv),
                                                        "overview", output, errors);
        }

        ~Help () override { option::reset_container (); }
    };

} // end anonymous namespace

TEST_F (Help, Empty) {
    string_stream output;
    string_stream errors;
    bool res = this->parse_command_line_options (output, errors);
    EXPECT_FALSE (res);
    EXPECT_EQ (output.str (), "OVERVIEW: overview\nUSAGE: program\n");
    EXPECT_EQ (errors.str (), "");
}

TEST_F (Help, HasSwitches) {
    {
        opt<std::string> option1{"arg1", positional};
        alias option2 ("alias1", aliasopt{option1});
        option::options_container container{&option1, &option2};
        EXPECT_FALSE (details::has_switches (nullptr, container));
    }
    {
        opt<std::string> option3{"arg2"};
        option::options_container container{&option3};
        EXPECT_TRUE (details::has_switches (nullptr, container));
    }
}

// categories_collection build_categories (option const * const self, option::options_container
// const & all);

TEST_F (Help, BuildDefaultCategoryOnly) {
    {
        opt<std::string> option1{"arg1", positional};
        opt<std::string> option2{"arg2"};
        option::options_container container{&option1, &option2};
        details::categories_collection const actual =
            details::build_categories (nullptr, container);

        ASSERT_EQ (actual.size (), 1U);
        auto const & first = *actual.begin ();
        EXPECT_EQ (first.first, nullptr);
        EXPECT_THAT (first.second, testing::ElementsAre (&option2));
    }
}

TEST_F (Help, BuildTwoCategories) {
    opt<std::string> option1{"arg1", positional};
    opt<std::string> option2{"arg2"};
    option_category category{"category"};
    opt<std::string> option3{"arg3", cat (category)};

    option::options_container container{&option1, &option2, &option3};
    details::categories_collection const actual = details::build_categories (nullptr, container);

    ASSERT_EQ (actual.size (), 2U);
    auto it = std::begin (actual);
    EXPECT_EQ (it->first, nullptr);
    EXPECT_THAT (it->second, testing::ElementsAre (&option2));
    ++it;
    EXPECT_EQ (it->first, &category);
    EXPECT_THAT (it->second, testing::ElementsAre (&option3));
}

TEST_F (Help, SwitchStrings) {
    // auto get_switch_strings (options_set const & ops) -> switch_strings {

    // This option has a name in Katakana to verify that we are counting unicode code-points and not
    // UTF-8 code-units.
    pstore::gsl::czstring const name = "\xE3\x82\xAA"  // KATAKANA LETTER O
                                       "\xE3\x83\x97"  // KATAKANA LETTER PU
                                       "\xE3\x82\xB7"  // KATAKANA LETTER SI
                                       "\xE3\x83\xA7"  // KATAKANA LETTER SMALL YO
                                       "\xE3\x83\xB3"; // KATAKANA LETTER N
    opt<std::string> option1{name};
    details::options_set options{&option1};
    details::switch_strings const actual = details::get_switch_strings (options);

    ASSERT_EQ (actual.size (), 1U);
    auto it = std::begin (actual);
    EXPECT_EQ (it->first, &option1);
    ASSERT_EQ (it->second.size (), 1U);
    EXPECT_EQ (it->second[0], std::make_tuple ("--"s + name + "=<str>", std::size_t{13}));
}
