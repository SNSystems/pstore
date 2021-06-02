//===- unittests/json/test_comment.cpp ------------------------------------===//
//*                                           _    *
//*   ___ ___  _ __ ___  _ __ ___   ___ _ __ | |_  *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \ '_ \| __| *
//* | (_| (_) | | | | | | | | | | |  __/ | | | |_  *
//*  \___\___/|_| |_| |_|_| |_| |_|\___|_| |_|\__| *
//*                                                *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/json/json.hpp"

#include "callbacks.hpp"

using namespace std::string_literals;
using namespace pstore;

namespace {

    class Comment : public ::testing::Test {
    public:
        Comment ()
                : proxy_{callbacks_} {}

    protected:
        testing::StrictMock<mock_json_callbacks> callbacks_;
        callbacks_proxy<mock_json_callbacks> proxy_;
    };

} // end anonymous namespace

TEST_F (Comment, BashDisabled) {
    json::parser<decltype (proxy_)> p = json::make_parser (proxy_);
    p.input ("# comment\nnull"s).eof ();
    EXPECT_TRUE (p.has_error ());
}

TEST_F (Comment, BashSingleLeading) {
    EXPECT_CALL (callbacks_, null_value ()).Times (1);

    json::parser<decltype (proxy_)> p = json::make_parser (proxy_, json::extensions::bash_comments);
    p.input ("# comment\nnull"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Comment, BashMultipleLeading) {
    EXPECT_CALL (callbacks_, null_value ()).Times (1);

    json::parser<decltype (proxy_)> p = json::make_parser (proxy_, json::extensions::bash_comments);
    p.input ("# comment\n\n    # remark\nnull"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Comment, BashTrailing) {
    EXPECT_CALL (callbacks_, null_value ()).Times (1);

    json::parser<decltype (proxy_)> p = json::make_parser (proxy_, json::extensions::bash_comments);
    p.input ("null # comment"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Comment, BashInsideArray) {
    using testing::_;
    EXPECT_CALL (callbacks_, begin_array ()).Times (1);
    EXPECT_CALL (callbacks_, uint64_value (_)).Times (2);
    EXPECT_CALL (callbacks_, end_array ()).Times (1);

    json::parser<decltype (proxy_)> p = json::make_parser (proxy_, json::extensions::bash_comments);
    p.input (R"([#comment
1,     # comment containing #
2 # comment
]
)"s)
        .eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Comment, SingleLineDisabled) {
    json::parser<decltype (proxy_)> p = json::make_parser (proxy_);
    p.input ("// comment\nnull"s).eof ();
    EXPECT_TRUE (p.has_error ());
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_token));
}

TEST_F (Comment, SingleLineSingleLeading) {
    EXPECT_CALL (callbacks_, null_value ()).Times (1);

    json::parser<decltype (proxy_)> p =
        json::make_parser (proxy_, json::extensions::single_line_comments);
    p.input ("// comment\nnull"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Comment, SingleLineMultipleLeading) {
    EXPECT_CALL (callbacks_, null_value ()).Times (1);

    json::parser<decltype (proxy_)> p =
        json::make_parser (proxy_, json::extensions::single_line_comments);
    p.input ("// comment\n\n    // remark\nnull"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Comment, SingleLineTrailing) {
    EXPECT_CALL (callbacks_, null_value ()).Times (1);

    json::parser<decltype (proxy_)> p =
        json::make_parser (proxy_, json::extensions::single_line_comments);
    p.input ("null // comment"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Comment, SingleLineInsideArray) {
    using testing::_;
    EXPECT_CALL (callbacks_, begin_array ()).Times (1);
    EXPECT_CALL (callbacks_, uint64_value (_)).Times (2);
    EXPECT_CALL (callbacks_, end_array ()).Times (1);

    json::parser<decltype (proxy_)> p =
        json::make_parser (proxy_, json::extensions::single_line_comments);
    p.input (R"([//comment
1,    // comment containing //
2 // comment
]
)"s)
        .eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Comment, SingleLineRowCounting) {
    using testing::_;
    EXPECT_CALL (callbacks_, begin_array ()).Times (1);
    EXPECT_CALL (callbacks_, uint64_value (_)).Times (2);
    EXPECT_CALL (callbacks_, end_array ()).Times (1);

    json::parser<decltype (proxy_)> p =
        json::make_parser (proxy_, json::extensions::single_line_comments);
    p.input (R"([ //comment
1, // comment
2 // comment
] // comment
// comment
)"s)
        .eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
    EXPECT_EQ (p.coordinate (), (json::coord{1, 6}));
}

TEST_F (Comment, MultiLineDisabled) {
    json::parser<decltype (proxy_)> p = json::make_parser (proxy_);
    p.input ("// comment\nnull"s).eof ();
    EXPECT_TRUE (p.has_error ());
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_token));
}

TEST_F (Comment, MultiLineSingleLeading) {
    EXPECT_CALL (callbacks_, null_value ()).Times (1);

    json::parser<decltype (proxy_)> p =
        json::make_parser (proxy_, json::extensions::multi_line_comments);
    p.input ("/* comment */\nnull"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Comment, MultiLineMultipleLeading) {
    EXPECT_CALL (callbacks_, null_value ()).Times (1);

    json::parser<decltype (proxy_)> p =
        json::make_parser (proxy_, json::extensions::multi_line_comments);
    p.input ("/* comment\ncomment */\nnull"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Comment, MultiLineTrailing) {
    EXPECT_CALL (callbacks_, null_value ()).Times (1);

    json::parser<decltype (proxy_)> p =
        json::make_parser (proxy_, json::extensions::multi_line_comments);
    p.input ("null\n/* comment */\n"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Comment, MultiLineInsideArray) {
    using testing::_;
    EXPECT_CALL (callbacks_, begin_array ()).Times (1);
    EXPECT_CALL (callbacks_, uint64_value (_)).Times (2);
    EXPECT_CALL (callbacks_, end_array ()).Times (1);

    json::parser<decltype (proxy_)> p =
        json::make_parser (proxy_, json::extensions::multi_line_comments);
    p.input (R"([ /* comment */
1,    /* comment containing / * */
2 /* comment */
]
)"s)
        .eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (Comment, MultiLineRowCounting) {
    using testing::_;
    EXPECT_CALL (callbacks_, begin_array ()).Times (1);
    EXPECT_CALL (callbacks_, uint64_value (_)).Times (2);
    EXPECT_CALL (callbacks_, end_array ()).Times (1);

    json::parser<decltype (proxy_)> p =
        json::make_parser (proxy_, json::extensions::multi_line_comments);
    p.input (R"([ /*comment */
1, /* comment
comment
*/
2 /* comment */
]
/* comment
comment */
)"s)
        .eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
    EXPECT_EQ (p.coordinate (), (json::coord{1, 9}));
}

// A missing multi-line comment close is currently ignored. It could reasonably raise an error, but
// at this point I've chosen not to do so.
TEST_F (Comment, MultiLineUnclosed) {
    using testing::_;
    EXPECT_CALL (callbacks_, null_value ()).Times (1);

    json::parser<decltype (proxy_)> p =
        json::make_parser (proxy_, json::extensions::multi_line_comments);
    p.input ("null /*comment"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
    EXPECT_EQ (p.coordinate (), (json::coord{15, 1}));
}
