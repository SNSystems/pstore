//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===- unittests/pstore_json/test_json.cpp --------------------------------===//
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
#include "pstore/json/json.hpp"
#include <stack>
#include "callbacks.hpp"

using namespace pstore;
using testing::StrictMock;
using testing::DoubleEq;

namespace {

    class json_out_callbacks {
    public:
        using result_type = std::string;
        result_type result () { return out_; }

        void string_value (std::string const & s) { append ('"' + s + '"'); }
        void integer_value (long v) { append (std::to_string (v)); }
        void float_value (double v) { append (std::to_string (v)); }
        void boolean_value (bool v) { append (v ? "true" : "false"); }
        void null_value () { append ("null"); }

        void begin_array () { append ("["); }
        void end_array () { append ("]"); }

        void begin_object () { append ("{"); }
        void end_object () { append ("}"); }

    private:
        template <typename StringType>
        void append (StringType && s) {
            if (out_.length () == 0) {
                out_ += ' ';
            }
            out_ += s;
        }

        std::string out_;
    };


    class Json : public ::testing::Test {
    protected:
        static void check_error (char const * src, json::error_code err) {
            ASSERT_NE (err, json::error_code::none);
            json::parser<json_out_callbacks> p;
            p.parse (src);
            std::string res = p.eof ();
            EXPECT_EQ (res, "");
            EXPECT_NE (p.last_error (), std::make_error_code (json::error_code::none));
        }
    };

} // end anonymous namespace

TEST_F (Json, Empty) {
    check_error ("", json::error_code::expected_token);
    check_error ("   \t    ", json::error_code::expected_token);
}

TEST_F (Json, Null) {
    StrictMock<mock_json_callbacks> callbacks;
    callbacks_proxy<mock_json_callbacks> proxy (callbacks);
    EXPECT_CALL (callbacks, null_value ()).Times (1);

    json::parser<decltype (proxy)> p (proxy);
    p.parse (" null ");
    p.eof ();
}

TEST_F (Json, Move) {
    StrictMock<mock_json_callbacks> callbacks;
    callbacks_proxy<mock_json_callbacks> proxy (callbacks);
    EXPECT_CALL (callbacks, null_value ()).Times (1);

    json::parser<decltype (proxy)> p (proxy);
    // Move to a new parser instance ('p2') from 'p' and make sure that 'p2' is usuable.
    auto p2 = std::move (p);
    p2.parse (" null ");
    p2.eof ();
}

TEST_F (Json, TwoKeywords) {
    check_error (" true false ", json::error_code::unexpected_extra_input);
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
        JsonBoolean () : proxy_{callbacks_} {}
    protected:
        StrictMock<mock_json_callbacks> callbacks_;
        callbacks_proxy<mock_json_callbacks> proxy_;
    };

} // end anonymous namespace

TEST_F (JsonBoolean, True) {
    EXPECT_CALL (callbacks_, boolean_value (true)).Times (1);

    json::parser<decltype (proxy_)> p = json::make_parser (proxy_);
    p.parse ("true");
    p.eof ();
    EXPECT_FALSE (p.has_error());
}

TEST_F (JsonBoolean, False) {
    StrictMock<mock_json_callbacks> callbacks;
    callbacks_proxy<mock_json_callbacks> proxy (callbacks);
    EXPECT_CALL (callbacks, boolean_value (false)).Times (1);

    json::parser<decltype (proxy)> p = json::make_parser (proxy);
    p.parse (" false ");
    p.eof ();
    EXPECT_FALSE (p.has_error());
}


//*     _              ___ _       _            *
//*  _ | |___ ___ _ _ / __| |_ _ _(_)_ _  __ _  *
//* | || (_-</ _ \ ' \\__ \  _| '_| | ' \/ _` | *
//*  \__//__/\___/_||_|___/\__|_| |_|_||_\__, | *
//*                                      |___/  *

namespace {

    class JsonString : public Json {
    protected:
        void check (char const * src, char const * expected) {
            ASSERT_NE (src, nullptr);
            ASSERT_NE (expected, nullptr);

            StrictMock<mock_json_callbacks> callbacks;
            callbacks_proxy<mock_json_callbacks> proxy (callbacks);
            EXPECT_CALL (callbacks, string_value (expected)).Times (1);

            json::parser<decltype (proxy)> p = json::make_parser (proxy);
            p.parse (src);
            p.eof ();
            EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
        }
    };

} // end anonymous namespace

TEST_F (JsonString, Simple) {
    this->check ("\"\"", "");
    this->check ("\"hello\"", "hello");
}

TEST_F (JsonString, Unterminated) {
    this->check_error ("\"hello", json::error_code::expected_close_quote);
}

TEST_F (JsonString, EscapeN) {
    this->check ("\"a\\n\"", "a\n");
}

TEST_F (JsonString, BadEscape) {
    this->check_error ("\"a\\qb\"", json::error_code::invalid_escape_char);
}

TEST_F (JsonString, BackslashQuoteUnterminated) {
    this->check_error ("\"a\\\"", json::error_code::expected_close_quote);
}

TEST_F (JsonString, TrailingBackslashUnterminated) {
    this->check_error ("\"a\\", json::error_code::invalid_escape_char);
}

TEST_F (JsonString, GCleffUtf8) {
    // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed in UTF-8
    this->check ("\"\xF0\x9D\x84\x9E\"", "\xF0\x9D\x84\x9E");
}

TEST_F (JsonString, SlashUnicodeUpper) {
    this->check ("\"\\u002F\"", "/");
}

TEST_F (JsonString, TwoUtf16Chars) {
    // Encoding for TURNED AMPERSAND (U+214B) followed by KATAKANA LETTER SMALL A (u+30A1)
    // expressed as a pair of UTF-16 characters.
    this->check ("\"\\u214B\\u30A1\"", "\xE2\x85\x8B\xE3\x82\xA1");
}

TEST_F (JsonString, Utf16Surrogates) {
    // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as a UTF-16
    // surrogate pair.
    this->check ("\"\\uD834\\uDD1E\"", "\xF0\x9D\x84\x9E");
}

TEST_F (JsonString, Utf16HighWithNoLowSurrogate) {
    // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
    this->check_error ("\"\\uD834\\u30A1\"", json::error_code::bad_unicode_code_point);
}

TEST_F (JsonString, Utf16HighFollowedByUtf8Char) {
    // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
    this->check_error ("\"\\uD834!\"", json::error_code::bad_unicode_code_point);
}

TEST_F (JsonString, Utf16HighWithMissingLowSurrogate) {
    // Encoding for MUSICAL SYMBOL G CLEF (U+1D11E) expressed as a UTF-16
    // surrogate pair.
    this->check_error ("\"\\uDD1E\\u30A1\"", json::error_code::bad_unicode_code_point);
}

TEST_F (JsonString, ControlCharacter) {
    this->check_error ("\"\t\"", json::error_code::bad_unicode_code_point);

    this->check ("\"\\u0009\"", "\t");
}

TEST_F (JsonString, Utf16LowWithNoHighSurrogate) {
    // UTF-16 high surrogate followed by non-surrogate UTF-16 hex code point.
    this->check_error ("\"\\uD834\"", json::error_code::bad_unicode_code_point);
}

TEST_F (JsonString, SlashBadHexChar) {
    this->check_error ("\"\\u00xF\"", json::error_code::invalid_escape_char);
}

TEST_F (JsonString, PartialHexChar) {
    this->check_error ("\"\\u00", json::error_code::invalid_escape_char);
}


//*     _                _                      *
//*  _ | |___ ___ _ _   /_\  _ _ _ _ __ _ _  _  *
//* | || (_-</ _ \ ' \ / _ \| '_| '_/ _` | || | *
//*  \__//__/\___/_||_/_/ \_\_| |_| \__,_|\_, | *
//*                                       |__/  *

namespace {

    class JsonArray : public ::testing::Test {
    public:
        JsonArray () : proxy_{callbacks_} {}
    protected:
        StrictMock<mock_json_callbacks> callbacks_;
        callbacks_proxy<mock_json_callbacks> proxy_;
    };

} // end anonymous namespace

TEST_F (JsonArray, Empty) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.parse (" [ ] ");
    p.eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
}

TEST_F (JsonArray, ArrayNoCloseBracket) {
    mock_json_callbacks callbacks;
    auto p = json::make_parser (callbacks_proxy<mock_json_callbacks> (callbacks));
    p.parse ("[");
    p.eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_array_member));
}

TEST_F (JsonArray, SingleElement) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, integer_value (1L)).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.parse ("[ 1 ]");
    p.eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
}

TEST_F (JsonArray, SingleStringElement) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, string_value ("a")).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.parse ("[\"a\"]");
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
}

TEST_F (JsonArray, ZeroExpPlus1) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, float_value (DoubleEq (0.0))).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.parse ("[0e+1]");
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
}

TEST_F (JsonArray, SimpleFloat) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, float_value (DoubleEq (1.234))).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.parse ("[1.234]");
    p.eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
}

TEST_F (JsonArray, MinusZero) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, integer_value (0)).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.parse ("[-0]");
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
}

TEST_F (JsonArray, TwoElements) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, integer_value (1)).Times (1);
        EXPECT_CALL (callbacks_, string_value ("hello")).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.parse ("[ 1 , \"hello\" ]");
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::none));
}

TEST_F (JsonArray, MisplacedComma) {
    {
        json::parser<json_out_callbacks> p;
        p.parse ("[,");
        p.eof ();
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_token));
    }
    {
        json::parser<json_out_callbacks> p;
        p.parse ("[,]");
        p.eof ();
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_token));
    }
    {
        json::parser<json_out_callbacks> p;
        p.parse ("[\"\",]");
        p.eof ();
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_token));
    }
    {
        json::parser<json_out_callbacks> p;
        p.parse ("[,1");
        p.eof ();
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_token));
    }
    {
        json::parser<json_out_callbacks> p;
        p.parse ("[1,,2]");
        p.eof ();
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_token));
    }
    {
        json::parser<json_out_callbacks> p;
        p.parse ("[1 true]");
        p.eof ();
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_array_member));
    }
}

TEST_F (JsonArray, NestedError) {
    {
        json::parser<json_out_callbacks> p;
        p.parse ("[[no");
        p.eof ();
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
    }
    {
        json::parser<json_out_callbacks> p;
        p.parse ("[[null");
        p.eof ();
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_array_member));
    }
}

TEST_F (JsonArray, Nested) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (2);
        EXPECT_CALL (callbacks_, null_value ()).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (2);
    }
    auto p = json::make_parser (proxy_);
    p.parse ("[[null]]");
    p.eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonArray, Nested2) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (2);
        EXPECT_CALL (callbacks_, null_value ()).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, integer_value (1)).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (2);
    }
    auto p = json::make_parser (proxy_);
    p.parse ("[[null], [1]]");
    p.eof ();
    EXPECT_FALSE (p.has_error ());
}


//*     _              ___  _     _        _    *
//*  _ | |___ ___ _ _ / _ \| |__ (_)___ __| |_  *
//* | || (_-</ _ \ ' \ (_) | '_ \| / -_) _|  _| *
//*  \__//__/\___/_||_\___/|_.__// \___\__|\__| *
//*                            |__/             *

namespace {

    class JsonObject : public testing::Test {
    public:
        JsonObject () : proxy_{callbacks_} {}
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
    p.parse ("{}");
    p.eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonObject, SingleKvp) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_object ()).Times (1);
        EXPECT_CALL (callbacks_, string_value ("a")).Times (1);
        EXPECT_CALL (callbacks_, integer_value (1L)).Times (1);
        EXPECT_CALL (callbacks_, end_object ()).Times (1);
    }

    auto p = json::make_parser (proxy_);
    p.parse ("{\"a\":1}");
    p.eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonObject, TwoKvps) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_object ()).Times (1);
        EXPECT_CALL (callbacks_, string_value ("a")).Times (1);
        EXPECT_CALL (callbacks_, integer_value (1)).Times (1);
        EXPECT_CALL (callbacks_, string_value ("b")).Times (1);
        EXPECT_CALL (callbacks_, boolean_value (true)).Times (1);
        EXPECT_CALL (callbacks_, end_object ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.parse ("{\"a\":1, \"b\" : true }");
    p.eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonObject, ArrayValue) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_object ()).Times (1);
        EXPECT_CALL (callbacks_, string_value ("a")).Times (1);
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, integer_value (1L)).Times (1);
        EXPECT_CALL (callbacks_, integer_value (2L)).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
        EXPECT_CALL (callbacks_, end_object ()).Times (1);
    }

    auto p = json::make_parser (proxy_);
    p.parse ("{\"a\": [1,2]}");
    p.eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonObject, MisplacedComma) {
    {
        json::parser<json_out_callbacks> p1;
        p1.parse ("{\"a\":1,}");
        p1.eof ();
        EXPECT_EQ (p1.last_error (), std::make_error_code (json::error_code::expected_token));
    }
    {
        json::parser<json_out_callbacks> p2;
        p2.parse ("{\"a\":1 \"b\":1}");
        p2.eof ();
        EXPECT_EQ (p2.last_error (), std::make_error_code (json::error_code::expected_object_member));
    }
    {
        json::parser<json_out_callbacks> p3;
        p3.parse ("{\"a\":1,,\"b\":1}");
        p3.eof ();
        EXPECT_EQ (p3.last_error (), std::make_error_code (json::error_code::expected_token));
    }
}

TEST_F (JsonObject, KeyIsNotString) {
    json::parser<json_out_callbacks> p;
    p.parse ("{{}:{}}");
    p.eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_string));
}

TEST_F (JsonObject, BadNestedObject) {
    json::parser<json_out_callbacks> p;
    p.parse ("{\"a\":nu}");
    p.eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
}

// eof: unittests/pstore_json/test_json.cpp
