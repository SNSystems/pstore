//===- unittests/json/test_json.cpp ---------------------------------------===//
//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/json/json.hpp"
#include <stack>
#include "callbacks.hpp"

using namespace std::string_literals;
using namespace pstore;
using testing::DoubleEq;
using testing::StrictMock;

namespace {



    class Json : public ::testing::Test {
    protected:
        static void check_error (std::string const & src, json::error_code err) {
            ASSERT_NE (err, json::error_code::none);
            json::parser<json_out_callbacks> p;
            std::string const res = p.input (src).eof ();
            EXPECT_EQ (res, "");
            EXPECT_NE (p.last_error (), make_error_code (json::error_code::none));
        }
    };

} // end anonymous namespace

TEST_F (Json, Empty) {
    json::parser<json_out_callbacks> p;
    p.input (std::string{}).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_token));
    EXPECT_EQ (p.coordinate (), (json::coord{1U, 1U}));
}

TEST_F (Json, StringAndIteratorAPI) {
    std::string const src = "null";
    {
        json::parser<json_out_callbacks> p1;
        std::string const res = p1.input (src).eof ();
        EXPECT_FALSE (p1.has_error ());
        EXPECT_EQ (res, "null");
        EXPECT_EQ (p1.coordinate (), (json::coord{5U, 1U}));
    }
    {
        json::parser<json_out_callbacks> p2;
        std::string const res = p2.input (std::begin (src), std::end (src)).eof ();
        EXPECT_FALSE (p2.has_error ());
        EXPECT_EQ (res, "null");
        EXPECT_EQ (p2.coordinate (), (json::coord{5U, 1U}));
    }
}

TEST_F (Json, Whitespace) {
    {
        json::parser<json_out_callbacks> p1;
        std::string const res = p1.input ("   \t    null"s).eof ();
        EXPECT_FALSE (p1.has_error ());
        EXPECT_EQ (res, "null");
        EXPECT_EQ (p1.coordinate (), (json::coord{13U, 1U}));
    }

    auto const cr = "\r"s;
    auto const lf = "\n"s;
    auto const crlf = cr + lf;
    auto const keyword = "null"s;
    auto const xord = static_cast<unsigned> (keyword.length ()) + 1U;

    {
        json::parser<json_out_callbacks> p2;
        p2.input (lf + lf + keyword); // POSIX-style line endings
        std::string const res = p2.eof ();
        EXPECT_FALSE (p2.has_error ());
        EXPECT_EQ (res, keyword);
        EXPECT_EQ (p2.coordinate (), (json::coord{xord, 3U}));
    }
    {
        json::parser<json_out_callbacks> p3;
        p3.input (cr + cr + keyword); // MacOS Classic line endings
        std::string const res = p3.eof ();
        EXPECT_FALSE (p3.has_error ());
        EXPECT_EQ (res, keyword);
        EXPECT_EQ (p3.coordinate (), (json::coord{xord, 3U}));
    }
    {
        json::parser<json_out_callbacks> p4;
        p4.input (crlf + crlf + keyword); // Windows-style CRLF
        std::string const res = p4.eof ();
        EXPECT_FALSE (p4.has_error ());
        EXPECT_EQ (res, keyword);
        EXPECT_EQ (p4.coordinate (), (json::coord{xord, 3U}));
    }
    {
        json::parser<json_out_callbacks> p5;
        // Nobody's line-endings. Each counts as a new line. Note that the middle cr+lf pair will
        // match a single Windows crlf.
        std::string const res = p5.input (lf + cr + lf + cr + keyword).eof ();
        EXPECT_FALSE (p5.has_error ());
        EXPECT_EQ (res, keyword);
        EXPECT_EQ (p5.coordinate (), (json::coord{xord, 4U}));
    }
    {
        json::parser<json_out_callbacks> p6;
        p6.input (lf + lf + crlf + cr + keyword); // A groovy mixture of line-ending characters.
        std::string const res = p6.eof ();
        EXPECT_FALSE (p6.has_error ());
        EXPECT_EQ (res, "null");
        EXPECT_EQ (p6.coordinate (), (json::coord{xord, 5U}));
    }
}

TEST_F (Json, Null) {
    StrictMock<mock_json_callbacks> callbacks;
    callbacks_proxy<mock_json_callbacks> proxy (callbacks);
    EXPECT_CALL (callbacks, null_value ()).Times (1);

    json::parser<decltype (proxy)> p (proxy);
    p.input (" null "s).eof ();
    EXPECT_FALSE (p.has_error ());
    EXPECT_EQ (p.coordinate (), (json::coord{7U, 1U}));
}

TEST_F (Json, Move) {
    StrictMock<mock_json_callbacks> callbacks;
    callbacks_proxy<mock_json_callbacks> proxy (callbacks);
    EXPECT_CALL (callbacks, null_value ()).Times (1);

    json::parser<decltype (proxy)> p (proxy);
    // Move to a new parser instance ('p2') from 'p' and make sure that 'p2' is usuable.
    auto p2 = std::move (p);
    p2.input (" null "s).eof ();
    EXPECT_FALSE (p2.has_error ());
    EXPECT_EQ (p2.coordinate (), (json::coord{7U, 1U}));
}

TEST_F (Json, TwoKeywords) {
    json::parser<json_out_callbacks> p;
    p.input (" true false "s);
    std::string const res = p.eof ();
    EXPECT_EQ (res, "");
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::unexpected_extra_input));
    EXPECT_EQ (p.coordinate (), (json::coord{7U, 1U}));
}

TEST_F (Json, BadKeyword) {
    check_error ("nu", json::error_code::expected_token);
    check_error ("bad", json::error_code::expected_token);
    check_error ("fal", json::error_code::expected_token);
    check_error ("falsehood", json::error_code::unexpected_extra_input);
}


//*     _              ___           _                *
//*  _ | |___ ___ _ _ | _ ) ___  ___| |___ __ _ _ _   *
//* | || (_-</ _ \ ' \| _ \/ _ \/ _ \ / -_) _` | ' \  *
//*  \__//__/\___/_||_|___/\___/\___/_\___\__,_|_||_| *
//*                                                   *

namespace {

    class JsonBoolean : public Json {
    public:
        JsonBoolean ()
                : proxy_{callbacks_} {}

    protected:
        StrictMock<mock_json_callbacks> callbacks_;
        callbacks_proxy<mock_json_callbacks> proxy_;
    };

} // end anonymous namespace

TEST_F (JsonBoolean, True) {
    EXPECT_CALL (callbacks_, boolean_value (true)).Times (1);

    json::parser<decltype (proxy_)> p = json::make_parser (proxy_);
    p.input ("true"s);
    p.eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonBoolean, False) {
    StrictMock<mock_json_callbacks> callbacks;
    callbacks_proxy<mock_json_callbacks> proxy (callbacks);
    EXPECT_CALL (callbacks, boolean_value (false)).Times (1);

    json::parser<decltype (proxy)> p = json::make_parser (proxy);
    p.input (" false "s);
    p.eof ();
    EXPECT_FALSE (p.has_error ());
}
