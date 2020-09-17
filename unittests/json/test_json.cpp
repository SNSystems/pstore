//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===- unittests/json/test_json.cpp ---------------------------------------===//
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
#include "pstore/json/json.hpp"
#include <stack>
#include "callbacks.hpp"

using namespace std::string_literals;
using namespace pstore;
using testing::DoubleEq;
using testing::StrictMock;

namespace {

    class json_out_callbacks {
    public:
        using result_type = std::string;
        result_type result () { return out_; }

        std::error_code string_value (std::string const & s) {
            append ('"' + s + '"');
            return {};
        }
        std::error_code int64_value (std::int64_t v) {
            append (std::to_string (v));
            return {};
        }
        std::error_code uint64_value (std::uint64_t v) {
            append (std::to_string (v));
            return {};
        }
        std::error_code double_value (double v) {
            append (std::to_string (v));
            return {};
        }
        std::error_code boolean_value (bool v) {
            append (v ? "true" : "false");
            return {};
        }
        std::error_code null_value () {
            append ("null");
            return {};
        }

        std::error_code begin_array () {
            append ("[");
            return {};
        }
        std::error_code end_array () {
            append ("]");
            return {};
        }

        std::error_code begin_object () {
            append ("{");
            return {};
        }
        std::error_code key (std::string const & s) {
            string_value (s);
            return {};
        }
        std::error_code end_object () {
            append ("}");
            return {};
        }

    private:
        template <typename StringType>
        void append (StringType && s) {
            if (out_.length () > 0) {
                out_ += ' ';
            }
            out_ += s;
        }

        std::string out_;
    };


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


//*     _              ___ _       _            *
//*  _ | |___ ___ _ _ / __| |_ _ _(_)_ _  __ _  *
//* | || (_-</ _ \ ' \\__ \  _| '_| | ' \/ _` | *
//*  \__//__/\___/_||_|___/\__|_| |_|_||_\__, | *
//*                                      |___/  *

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


//*     _                _                      *
//*  _ | |___ ___ _ _   /_\  _ _ _ _ __ _ _  _  *
//* | || (_-</ _ \ ' \ / _ \| '_| '_/ _` | || | *
//*  \__//__/\___/_||_/_/ \_\_| |_| \__,_|\_, | *
//*                                       |__/  *

namespace {

    class JsonArray : public ::testing::Test {
    public:
        JsonArray ()
                : proxy_{callbacks_} {}

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
    p.input ("[\n]\n"s);
    p.eof ();
    EXPECT_FALSE (p.last_error ()) << "Expected the parse to succeed";
    EXPECT_EQ (p.coordinate (), (json::coord{1U, 3U}));
}

TEST_F (JsonArray, BeginArrayReturnsError) {
    std::error_code const error = make_error_code (std::errc::io_error);
    using ::testing::Return;
    EXPECT_CALL (callbacks_, begin_array ()).WillOnce (Return (error));

    auto p = json::make_parser (proxy_);
    p.input ("[\n]\n"s);
    EXPECT_EQ (p.last_error (), error);
    EXPECT_EQ (p.coordinate (), (json::coord{1U, 1U}));
}

TEST_F (JsonArray, ArrayNoCloseBracket) {
    auto p = json::make_parser (json_out_callbacks{});
    p.input ("["s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_array_member));
}

TEST_F (JsonArray, SingleElement) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    std::string const input = "[ 1 ]";
    p.input (gsl::make_span (input));
    p.eof ();
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
    EXPECT_EQ (p.coordinate (), (json::coord{static_cast<unsigned> (input.length ()) + 1U, 1U}));
}

TEST_F (JsonArray, SingleStringElement) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, string_value ("a")).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.input (std::string{"[\"a\"]"});
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
}

TEST_F (JsonArray, ZeroExpPlus1) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, double_value (DoubleEq (0.0))).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.input ("[0e+1]"s);
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
}

TEST_F (JsonArray, SimpleFloat) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, double_value (DoubleEq (1.234))).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.input ("[1.234]"s).eof ();
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
}

TEST_F (JsonArray, MinusZero) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, int64_value (0)).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.input ("[-0]"s);
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
}

TEST_F (JsonArray, TwoElements) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
        EXPECT_CALL (callbacks_, string_value ("hello")).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
    }
    auto p = json::make_parser (proxy_);
    p.input (std::string{"[ 1 ,\n \"hello\" ]"});
    EXPECT_FALSE (p.last_error ()) << "Expected the parse error to be zero";
    EXPECT_EQ (p.coordinate (), (json::coord{11U, 2U}));
}

TEST_F (JsonArray, MisplacedComma) {
    {
        json::parser<json_out_callbacks> p;
        p.input ("[,"s);
        p.eof ();
        EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_token));
    }
    {
        json::parser<json_out_callbacks> p;
        p.input ("[,]"s);
        p.eof ();
        EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_token));
    }
    {
        json::parser<json_out_callbacks> p;
        p.input (std::string{"[\"\",]"});
        p.eof ();
        EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_token));
    }
    {
        json::parser<json_out_callbacks> p;
        p.input ("[,1"s);
        p.eof ();
        EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_token));
    }
    {
        json::parser<json_out_callbacks> p;
        p.input ("[1,,2]"s);
        p.eof ();
        EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_token));
    }
    {
        json::parser<json_out_callbacks> p;
        p.input ("[1 true]"s);
        p.eof ();
        EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_array_member));
    }
}

TEST_F (JsonArray, NestedError) {
    {
        json::parser<json_out_callbacks> p;
        p.input ("[[no"s);
        p.eof ();
        EXPECT_EQ (p.last_error (), make_error_code (json::error_code::unrecognized_token));
    }
    {
        json::parser<json_out_callbacks> p;
        p.input ("[[null"s);
        p.eof ();
        EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_array_member));
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
    p.input ("[[null]]"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonArray, Nested2) {
    {
        ::testing::InSequence _;
        EXPECT_CALL (callbacks_, begin_array ()).Times (2);
        EXPECT_CALL (callbacks_, null_value ()).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (1);
        EXPECT_CALL (callbacks_, begin_array ()).Times (1);
        EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
        EXPECT_CALL (callbacks_, end_array ()).Times (2);
    }
    auto p = json::make_parser (proxy_);
    p.input ("[[null], [1]]"s);
    p.eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonArray, TooDeeplyNested) {
    json::parser<json_out_callbacks> p;
    p.input (std::string (std::string::size_type{200}, '[')).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::nesting_too_deep));
}


//*     _              ___  _     _        _    *
//*  _ | |___ ___ _ _ / _ \| |__ (_)___ __| |_  *
//* | || (_-</ _ \ ' \ (_) | '_ \| / -_) _|  _| *
//*  \__//__/\___/_||_\___/|_.__// \___\__|\__| *
//*                            |__/             *

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
        json::parser<json_out_callbacks> p1;
        p1.input (std::string{"{\"a\":1,}"});
        p1.eof ();
        EXPECT_EQ (p1.last_error (), make_error_code (json::error_code::expected_token));
        EXPECT_EQ (p1.coordinate (), (json::coord{8U, 1U}));
    }
    {
        json::parser<json_out_callbacks> p2;
        p2.input (std::string{"{\"a\":1 \"b\":1}"});
        p2.eof ();
        EXPECT_EQ (p2.last_error (), make_error_code (json::error_code::expected_object_member));
        EXPECT_EQ (p2.coordinate (), (json::coord{8U, 1U}));
    }
    {
        json::parser<json_out_callbacks> p3;
        p3.input (std::string{"{\"a\":1,,\"b\":1}"});
        p3.eof ();
        EXPECT_EQ (p3.last_error (), make_error_code (json::error_code::expected_token));
        EXPECT_EQ (p3.coordinate (), (json::coord{8U, 1U}));
    }
}

TEST_F (JsonObject, KeyIsNotString) {
    json::parser<json_out_callbacks> p;
    p.input ("{{}:{}}"s);
    p.eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_string));
    EXPECT_EQ (p.coordinate (), (json::coord{2U, 1U}));
}

TEST_F (JsonObject, BadNestedObject) {
    json::parser<json_out_callbacks> p;
    p.input (std::string{"{\"a\":nu}"});
    p.eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::unrecognized_token));
}

TEST_F (JsonObject, TooDeeplyNested) {
    json::parser<json_out_callbacks> p;

    std::string input;
    for (auto ctr = 0U; ctr < 200U; ++ctr) {
        input += "{\"a\":";
    }
    p.input (input).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::nesting_too_deep));
}
