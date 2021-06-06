//===- unittests/json/test_string.cpp -------------------------------------===//
//*      _        _              *
//*  ___| |_ _ __(_)_ __   __ _  *
//* / __| __| '__| | '_ \ / _` | *
//* \__ \ |_| |  | | | | | (_| | *
//* |___/\__|_|  |_|_| |_|\__, | *
//*                       |___/  *
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
using testing::DoubleEq;
using testing::StrictMock;

namespace {

    class JsonString : public testing::Test {
    public:
        JsonString ()
                : proxy_{callbacks_} {}

    protected:
        StrictMock<mock_json_callbacks> callbacks_;
        callbacks_proxy<mock_json_callbacks> proxy_;
    };

} // end anonymous namespace

TEST_F (JsonString, Empty) {
    EXPECT_CALL (callbacks_, string_value ("")).Times (1);

    json::parser<decltype (proxy_)> p = json::make_parser (proxy_);
    p.input (R"("")"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
    EXPECT_EQ (p.coordinate (), (json::coord{3U, 1U}));
}

TEST_F (JsonString, Simple) {
    EXPECT_CALL (callbacks_, string_value ("hello")).Times (1);

    json::parser<decltype (proxy_)> p = json::make_parser (proxy_);
    p.input (R"("hello")"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
    EXPECT_EQ (p.coordinate (), (json::coord{8U, 1U}));
}

TEST_F (JsonString, Unterminated) {
    auto p = json::make_parser (proxy_);
    p.input (R"("hello)"s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_close_quote));
    EXPECT_EQ (p.coordinate (), (json::coord{7U, 1U}));
}

TEST_F (JsonString, EscapeN) {
    EXPECT_CALL (callbacks_, string_value ("a\n")).Times (1);

    auto p = json::make_parser (proxy_);
    p.input (R"("a\n")"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
    EXPECT_EQ (p.coordinate (), (json::coord{6U, 1U}));
}

TEST_F (JsonString, BadEscape1) {
    auto p = json::make_parser (proxy_);
    p.input (R"("a\qb")").eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::invalid_escape_char));
    EXPECT_EQ (p.coordinate (), (json::coord{4U, 1U}));
}

TEST_F (JsonString, BadEscape2) {
    auto p = json::make_parser (proxy_);
    p.input ("\"\\\xC3\xBF\"").eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::invalid_escape_char));
    EXPECT_EQ (p.coordinate (), (json::coord{4U, 1U}));
}

TEST_F (JsonString, BackslashQuoteUnterminated) {
    auto p = json::make_parser (proxy_);
    p.input (R"("a\")").eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_close_quote));
    EXPECT_EQ (p.coordinate (), (json::coord{5U, 1U}));
}

TEST_F (JsonString, TrailingBackslashUnterminated) {
    auto p = json::make_parser (proxy_);
    p.input (R"("a\)").eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_close_quote));
    EXPECT_EQ (p.coordinate (), (json::coord{4U, 1U}));
}

TEST_F (JsonString, GCleffUtf8) {
    // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed in UTF-8
    // Note that the 4 bytes making up the code point count as a single column.
    EXPECT_CALL (callbacks_, string_value ("\xF0\x9D\x84\x9E")).Times (1);

    auto p = json::make_parser (proxy_);
    p.input ("\"\xF0\x9D\x84\x9E\""s).eof ();
    EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
    EXPECT_EQ (p.coordinate (), (json::coord{4U, 1U}));
}

TEST_F (JsonString, SlashUnicodeUpper) {
    EXPECT_CALL (callbacks_, string_value ("/")).Times (1);

    auto p = json::make_parser (proxy_);
    p.input ("\"\\u002F\"").eof ();
    EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
    EXPECT_EQ (p.coordinate (), (json::coord{9U, 1U}));
}

TEST_F (JsonString, FourFs) {
    // Note that there is no unicode code-point at U+FFFF.
    EXPECT_CALL (callbacks_, string_value ("\xEF\xBF\xBF")).Times (1);

    auto p = json::make_parser (proxy_);
    p.input ("\"\\uFFFF\"").eof ();
    EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
    EXPECT_EQ (p.coordinate (), (json::coord{9U, 1U}));
}

TEST_F (JsonString, TwoUtf16Chars) {
    // Encoding for TURNED AMPERSAND (U+214B) followed by KATAKANA LETTER SMALL A (u+30A1)
    // expressed as a pair of UTF-16 characters.
    EXPECT_CALL (callbacks_, string_value ("\xE2\x85\x8B\xE3\x82\xA1")).Times (1);

    auto p = json::make_parser (proxy_);
    p.input (R"("\u214B\u30A1")").eof ();
    EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
    EXPECT_EQ (p.coordinate (), (json::coord{15U, 1U}));
}

TEST_F (JsonString, Utf16Surrogates) {
    // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as a UTF-16
    // surrogate pair.
    EXPECT_CALL (callbacks_, string_value ("\xF0\x9D\x84\x9E")).Times (1);

    auto p = json::make_parser (proxy_);
    p.input (R"("\uD834\uDD1E")").eof ();
    EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
    EXPECT_EQ (p.coordinate (), (json::coord{15U, 1U}));
}

TEST_F (JsonString, Utf16HighWithNoLowSurrogate) {
    // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
    auto p = json::make_parser (proxy_);
    p.input (R"("\uD834\u30A1")").eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::bad_unicode_code_point));
    EXPECT_EQ (p.coordinate (), (json::coord{13U, 1U}));
}

TEST_F (JsonString, Utf16HighFollowedByUtf8Char) {
    // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
    auto p = json::make_parser (proxy_);
    p.input (R"("\uD834!")").eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::bad_unicode_code_point));
    EXPECT_EQ (p.coordinate (), (json::coord{8U, 1U}));
}

TEST_F (JsonString, Utf16HighWithMissingLowSurrogate) {
    // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as a UTF-16
    // surrogate pair.
    auto p = json::make_parser (proxy_);
    p.input (R"("\uDD1E\u30A1")").eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::bad_unicode_code_point));
    EXPECT_EQ (p.coordinate (), (json::coord{7U, 1U}));
}

TEST_F (JsonString, ControlCharacter) {
    auto p = json::make_parser (proxy_);
    p.input ("\"\t\"").eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::bad_unicode_code_point));
    EXPECT_EQ (p.coordinate (), (json::coord{2U, 1U}));
}

TEST_F (JsonString, ControlCharacterUTF16) {
    EXPECT_CALL (callbacks_, string_value ("\t")).Times (1);

    auto p = json::make_parser (proxy_);
    p.input (R"("\u0009")").eof ();
    EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
    EXPECT_EQ (p.coordinate (), (json::coord{9U, 1U}));
}

TEST_F (JsonString, Utf16LowWithNoHighSurrogate) {
    // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
    auto p = json::make_parser (proxy_);
    p.input (R"("\uD834")").eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::bad_unicode_code_point));
    EXPECT_EQ (p.coordinate (), (json::coord{8U, 1U}));
}

TEST_F (JsonString, SlashBadHexChar) {
    auto p = json::make_parser (proxy_);
    p.input ("\"\\u00xF\"").eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::invalid_hex_char));
    EXPECT_EQ (p.coordinate (), (json::coord{6U, 1U}));
}

TEST_F (JsonString, PartialHexChar) {
    auto p = json::make_parser (proxy_);
    p.input (R"("\u00)").eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_close_quote));
    EXPECT_EQ (p.coordinate (), (json::coord{6U, 1U}));
}
