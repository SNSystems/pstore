//*      _        _              *
//*  ___| |_ _ __(_)_ __   __ _  *
//* / __| __| '__| | '_ \ / _` | *
//* \__ \ |_| |  | | | | | (_| | *
//* |___/\__|_|  |_|_| |_|\__, | *
//*                       |___/  *
//===- unittests/dump/test_string.cpp -------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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

/// \file string.cpp
/// \brief Unit tests for the YAML string output implementation.

#include "dump/value.hpp"
#include <numeric>
#include "gtest/gtest.h"

namespace {
    class StringFixture : public ::testing::Test {
    public:
        template <typename InputIterator>
        std::string convert (InputIterator begin, InputIterator end) const;

        std::string convert (std::string const & source) const;
        std::string convert (value::string const & s) const;
    };

    template <typename InputIterator>
    std::string StringFixture::convert (InputIterator begin, InputIterator end) const {
        return this->convert ({begin, end});
    }

    std::string StringFixture::convert (std::string const & source) const {
        return this->convert (value::string{source});
    }

    std::string StringFixture::convert (value::string const & s) const {
        std::ostringstream out;
        s.write (out);
        return out.str ();
    }
}

TEST_F (StringFixture, Empty) {
    std::string const actual = this->convert ("");
    EXPECT_EQ ("\"\"", actual);
}

TEST_F (StringFixture, EmptyForceQuoted) {
    value::string s{"", true};
    std::string const actual = this->convert (s);
    EXPECT_EQ ("\"\"", actual);
}


TEST_F (StringFixture, OneAsciiChar) {
    std::string const actual = this->convert ("a");
    EXPECT_EQ ("a", actual);
}

TEST_F (StringFixture, OneAsciiCharForceQuoted) {
    value::string s{"a", true};
    std::string const actual = this->convert (s);
    EXPECT_EQ ("\"a\"", actual);
}

TEST_F (StringFixture, WhiteSpaceOnly) {
    std::string const actual = this->convert ("  ");
    EXPECT_EQ ("\"  \"", actual);
}

TEST_F (StringFixture, StartsWithWhiteSpace) {
    std::string const actual = this->convert ("  a");
    EXPECT_EQ ("\"  a\"", actual);
}

TEST_F (StringFixture, EndsWithWhiteSpace) {
    std::string const actual = this->convert ("a  ");
    EXPECT_EQ ("\"a  \"", actual);
}

TEST_F (StringFixture, TabCrLf) {
    std::string const actual = this->convert ("\t\n\r");
    EXPECT_EQ ("\"\\t\\n\\r\"", actual);
}

TEST_F (StringFixture, StartsWithQuote) {
    std::string const actual = this->convert ("\"a");
    EXPECT_EQ ("\\\"a", actual);
}

TEST_F (StringFixture, StartsWithBang) {
    std::string const actual = this->convert ("!a");
    EXPECT_EQ ("\\!a", actual);
}

TEST_F (StringFixture, ContainsQuote) {
    std::string const actual = this->convert ("a\"a");
    EXPECT_EQ ("a\"a", actual);
}

TEST_F (StringFixture, ContainsBackslash) {
    std::string const actual = this->convert ("a\\a");
    EXPECT_EQ ("a\\\\a", actual);
}

TEST_F (StringFixture, JapaneseUTF8) {
    std::array<std::uint8_t, 9 * 3> chars{{
        0xE3, 0x81, 0x8A, // HIRAGANA LETTER O
        0xE3, 0x81, 0xAF, // HIRAGANA LETTER HA
        0xE3, 0x82, 0x88, // HIRAGANA LETTER YO
        0xE3, 0x81, 0x86, // HIRAGANA LETTER U
        0xE3, 0x81, 0x94, // HIRAGANA LETTER GO
        0xE3, 0x81, 0x96, // HIRAGANA LETTER ZA
        0xE3, 0x81, 0x84, // HIRAGANA LETTER I
        0xE3, 0x81, 0xBE, // HIRAGANA LETTER MA
        0xE3, 0x81, 0x99, // HIRAGANA LETTER SU
    }};

    std::string const expected = "\""
                                 "\\xE3\\x81\\x8A"
                                 "\\xE3\\x81\\xAF"
                                 "\\xE3\\x82\\x88"
                                 "\\xE3\\x81\\x86"
                                 "\\xE3\\x81\\x94"
                                 "\\xE3\\x81\\x96"
                                 "\\xE3\\x81\\x84"
                                 "\\xE3\\x81\\xBE"
                                 "\\xE3\\x81\\x99"
                                 "\"";

    std::string actual = this->convert (std::begin (chars), std::end (chars));
    EXPECT_EQ (expected, actual);
}

TEST_F (StringFixture, AllChars) {
    std::array<std::uint8_t, 256> chars;
    std::iota (std::begin (chars), std::end (chars), std::uint8_t{0});

    std::string const expected =
        "\""
        "\\0\\x01\\x02\\x03\\x04\\x05\\x06\\a\\b\\t\\n\\v\\f\\r\\x0E\\x0F"
        "\\x10\\x11\\x12\\x13\\x14\\x15\\x16\\x17\\x18\\x19\\x1A\\e\\x1C\\x1D\\x1E\\x1F"
        " !\\\"#$%&'()*+,-./0123456789:;<=>?@"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
        "abcdefghijklmnopqrstuvwxyz{|}~"
        "\\x7F"
        "\\x80\\x81\\x82\\x83\\x84\\x85\\x86\\x87\\x88\\x89\\x8A\\x8B\\x8C\\x8D\\x8E\\x8F"
        "\\x90\\x91\\x92\\x93\\x94\\x95\\x96\\x97\\x98\\x99\\x9A\\x9B\\x9C\\x9D\\x9E\\x9F"
        "\\xA0\\xA1\\xA2\\xA3\\xA4\\xA5\\xA6\\xA7\\xA8\\xA9\\xAA\\xAB\\xAC\\xAD\\xAE\\xAF"
        "\\xB0\\xB1\\xB2\\xB3\\xB4\\xB5\\xB6\\xB7\\xB8\\xB9\\xBA\\xBB\\xBC\\xBD\\xBE\\xBF"
        "\\xC0\\xC1\\xC2\\xC3\\xC4\\xC5\\xC6\\xC7\\xC8\\xC9\\xCA\\xCB\\xCC\\xCD\\xCE\\xCF"
        "\\xD0\\xD1\\xD2\\xD3\\xD4\\xD5\\xD6\\xD7\\xD8\\xD9\\xDA\\xDB\\xDC\\xDD\\xDE\\xDF"
        "\\xE0\\xE1\\xE2\\xE3\\xE4\\xE5\\xE6\\xE7\\xE8\\xE9\\xEA\\xEB\\xEC\\xED\\xEE\\xEF"
        "\\xF0\\xF1\\xF2\\xF3\\xF4\\xF5\\xF6\\xF7\\xF8\\xF9\\xFA\\xFB\\xFC\\xFD\\xFE\\xFF"
        "\"";

    std::string const actual = this->convert (std::begin (chars), std::end (chars));
    EXPECT_EQ (expected, actual);
}

// eof: unittests/dump/test_string.cpp
