//===- unittests/json/test_object.cpp -------------------------------------===//
//*        _     _           _    *
//*   ___ | |__ (_) ___  ___| |_  *
//*  / _ \| '_ \| |/ _ \/ __| __| *
//* | (_) | |_) | |  __/ (__| |_  *
//*  \___/|_.__// |\___|\___|\__| *
//*           |__/                *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/json/json.hpp"
#include "pstore/json/dom_types.hpp"

#include "callbacks.hpp"

using namespace std::string_literals;
using namespace pstore;
using testing::DoubleEq;
using testing::StrictMock;

namespace {

    class JsonObject : public testing::Test {
    public:
        JsonObject ()
                : proxy_{callbacks_} {}

    protected:
        StrictMock<mock_json_callbacks> callbacks_;
        callbacks_proxy<mock_json_callbacks> proxy_;
    };

} // end anonymous namespace


TEST_F (JsonObject, Empty) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_object ()).Times (1);
        EXPECT_CALL (callbacks_, end_object ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.input ("{\r\n}\n"s);
    p.eof ();
    EXPECT_FALSE (p.has_error ());
    EXPECT_EQ (p.coordinate (), (json::coord{1U, 3U}));
}

TEST_F (JsonObject, SingleKvp) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_object ()).Times (1);
        EXPECT_CALL (callbacks_, key ("a")).Times (1);
        EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
        EXPECT_CALL (callbacks_, end_object ()).Times (1);
    }

    auto p = json::make_parser (proxy_);
    p.input (std::string{"{\n\"a\" : 1\n}"});
    p.eof ();
    EXPECT_FALSE (p.has_error ());
    EXPECT_EQ (p.coordinate (), (json::coord{2U, 3U}));
}

TEST_F (JsonObject, SingleKvpBadEndObject) {
    std::error_code const end_object_error = make_error_code (std::errc::io_error);

    using ::testing::_;
    using ::testing::Return;
    EXPECT_CALL (callbacks_, begin_object ());
    EXPECT_CALL (callbacks_, key (_));
    EXPECT_CALL (callbacks_, uint64_value (_));
    EXPECT_CALL (callbacks_, end_object ()).WillOnce (Return (end_object_error));

    auto p = json::make_parser (proxy_);
    p.input (std::string{"{\n\"a\" : 1\n}"});
    p.eof ();
    EXPECT_TRUE (p.has_error ());
    EXPECT_EQ (p.last_error (), end_object_error)
        << "Expected the error to be propagated from the end_object() callback";
    EXPECT_EQ (p.coordinate (), (json::coord{1U, 3U}));
}

TEST_F (JsonObject, TwoKvps) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_object ()).Times (1);
        EXPECT_CALL (callbacks_, key ("a")).Times (1);
        EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
        EXPECT_CALL (callbacks_, key ("b")).Times (1);
        EXPECT_CALL (callbacks_, boolean_value (true)).Times (1);
        EXPECT_CALL (callbacks_, end_object ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.input (std::string{"{\"a\":1, \"b\" : true }"});
    p.eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonObject, DuplicateKeys) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_object ()).Times (1);
        EXPECT_CALL (callbacks_, key ("a")).Times (1);
        EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
        EXPECT_CALL (callbacks_, key ("a")).Times (1);
        EXPECT_CALL (callbacks_, boolean_value (true)).Times (1);
        EXPECT_CALL (callbacks_, end_object ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.input (std::string{"{\"a\":1, \"a\" : true }"});
    p.eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonObject, ArrayValue) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_object ()).Times (1);
        EXPECT_CALL (callbacks_, key ("a")).Times (1);
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
        EXPECT_CALL (callbacks_, uint64_value (2)).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
        EXPECT_CALL (callbacks_, end_object ()).Times (1);
    }

    auto p = json::make_parser (proxy_);
    p.input (std::string{"{\"a\": [1,2]}"});
    p.eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonObject, MisplacedComma) {
    {
        // An object with a trailing comma but with the extension disabled.
        json::parser<json::null_output> p1;
        p1.input (R"({"a":1,})"s).eof ();
        EXPECT_EQ (p1.last_error (), make_error_code (json::error_code::expected_token));
        EXPECT_EQ (p1.coordinate (), (json::coord{8U, 1U}));
    }
    {
        json::parser<json::null_output> p2;
        p2.input (R"({"a":1 "b":1})"s).eof ();
        EXPECT_EQ (p2.last_error (), make_error_code (json::error_code::expected_object_member));
        EXPECT_EQ (p2.coordinate (), (json::coord{8U, 1U}));
    }
    {
        json::parser<json::null_output> p3;
        p3.input (R"({"a":1,,"b":1})"s).eof ();
        EXPECT_EQ (p3.last_error (), make_error_code (json::error_code::expected_token));
        EXPECT_EQ (p3.coordinate (), (json::coord{8U, 1U}));
    }
}

TEST_F (JsonObject, TrailingCommaExtensionEnabled) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_object ()).Times (1);
        EXPECT_CALL (callbacks_, key ("a")).Times (1);
        EXPECT_CALL (callbacks_, uint64_value (16)).Times (1);
        EXPECT_CALL (callbacks_, key ("b")).Times (1);
        EXPECT_CALL (callbacks_, string_value ("c")).Times (1);
        EXPECT_CALL (callbacks_, end_object ()).Times (1);
    }

    // An object with a trailing comma but with the extension _enabled_. Note that there is
    // deliberate whitespace around the final comma.
    auto p = json::make_parser (proxy_, json::extensions::object_trailing_comma);
    p.input (R"({ "a":16, "b":"c" , })"s).eof ();
    EXPECT_FALSE (p.has_error ());
}


TEST_F (JsonObject, KeyIsNotString) {
    json::parser<json::null_output> p;
    p.input ("{{}:{}}"s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_string));
    EXPECT_EQ (p.coordinate (), (json::coord{2U, 1U}));
}

TEST_F (JsonObject, BadNestedObject) {
    json::parser<json::null_output> p;
    p.input (std::string{"{\"a\":nu}"}).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::unrecognized_token));
}

TEST_F (JsonObject, TooDeeplyNested) {
    json::parser<json::null_output> p;

    std::string input;
    for (auto ctr = 0U; ctr < 200U; ++ctr) {
        input += "{\"a\":";
    }
    p.input (input).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::nesting_too_deep));
}
