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

    class JsonString : public Json {
    protected:
        void check (std::string const & src, char const * expected, unsigned column) {
            ASSERT_NE (expected, nullptr);

            StrictMock<mock_json_callbacks> callbacks;
            callbacks_proxy<mock_json_callbacks> proxy (callbacks);
            EXPECT_CALL (callbacks, string_value (expected)).Times (1);

            json::parser<decltype (proxy)> p = json::make_parser (proxy);
            p.input (src);
            p.eof ();
            EXPECT_FALSE (p.has_error ()) << "Expected the parse to succeed";
            EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
            EXPECT_EQ (p.coordinate (), (json::coord{column, 1U}));
        }
    };

} // end anonymous namespace

TEST_F (JsonString, Simple) {
    {
        SCOPED_TRACE ("Empty string");
        this->check ("\"\"", "", 3U);
    }
    {
        SCOPED_TRACE ("Simple Hello");
        this->check ("\"hello\"", "hello", 8U);
    }
}

TEST_F (JsonString, Unterminated) {
    check_error ("\"hello", json::error_code::expected_close_quote);
}

TEST_F (JsonString, EscapeN) {
    this->check ("\"a\\n\"", "a\n", 6U);
}

TEST_F (JsonString, BadEscape) {
    check_error ("\"a\\qb\"", json::error_code::invalid_escape_char);
    check_error ("\"\\\xC3\xBF\"", json::error_code::invalid_escape_char);
}

TEST_F (JsonString, BackslashQuoteUnterminated) {
    check_error ("\"a\\\"", json::error_code::expected_close_quote);
}

TEST_F (JsonString, TrailingBackslashUnterminated) {
    check_error ("\"a\\", json::error_code::invalid_escape_char);
}

TEST_F (JsonString, GCleffUtf8) {
    // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed in UTF-8
    // Note that the 4 bytes making up the code point count as a single column.
    this->check ("\"\xF0\x9D\x84\x9E\"", "\xF0\x9D\x84\x9E", 4U);
}

TEST_F (JsonString, SlashUnicodeUpper) {
    this->check ("\"\\u002F\"", "/", 9U);
}

TEST_F (JsonString, FourFs) {
    // Note that there is no unicode code-point at U+FFFF.
    this->check ("\"\\uFFFF\"", "\xEF\xBF\xBF", 9U);
}

TEST_F (JsonString, TwoUtf16Chars) {
    // Encoding for TURNED AMPERSAND (U+214B) followed by KATAKANA LETTER SMALL A (u+30A1)
    // expressed as a pair of UTF-16 characters.
    this->check ("\"\\u214B\\u30A1\"", "\xE2\x85\x8B\xE3\x82\xA1", 15U);
}

TEST_F (JsonString, Utf16Surrogates) {
    // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as a UTF-16
    // surrogate pair.
    this->check ("\"\\uD834\\uDD1E\"", "\xF0\x9D\x84\x9E", 15U);
}

TEST_F (JsonString, Utf16HighWithNoLowSurrogate) {
    // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
    check_error ("\"\\uD834\\u30A1\"", json::error_code::bad_unicode_code_point);
}

TEST_F (JsonString, Utf16HighFollowedByUtf8Char) {
    // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
    check_error ("\"\\uD834!\"", json::error_code::bad_unicode_code_point);
}

TEST_F (JsonString, Utf16HighWithMissingLowSurrogate) {
    // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as a UTF-16
    // surrogate pair.
    check_error ("\"\\uDD1E\\u30A1\"", json::error_code::bad_unicode_code_point);
}

TEST_F (JsonString, ControlCharacter) {
    check_error ("\"\t\"", json::error_code::bad_unicode_code_point);
    this->check ("\"\\u0009\"", "\t", 9U);
}

TEST_F (JsonString, Utf16LowWithNoHighSurrogate) {
    // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
    check_error ("\"\\uD834\"", json::error_code::bad_unicode_code_point);
}

TEST_F (JsonString, SlashBadHexChar) {
    check_error ("\"\\u00xF\"", json::error_code::invalid_escape_char);
}

TEST_F (JsonString, PartialHexChar) {
    check_error ("\"\\u00", json::error_code::invalid_escape_char);
}
