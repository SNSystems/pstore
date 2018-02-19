//*        _    __  *
//*  _   _| |_ / _| *
//* | | | | __| |_  *
//* | |_| | |_|  _| *
//*  \__,_|\__|_|   *
//*                 *
//===- unittests/pstore_support/test_utf.cpp ------------------------------===//
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

#include "pstore_support/utf.hpp"

#include "gmock/gmock.h"
#include <cstddef>
#include <cstdint>
#include <limits>

TEST (Utf, LengthOfEmptySequenceIsZero) {
    ASSERT_EQ (0U, pstore::utf::length (nullptr, 0));
}

TEST (Utf, LengthOfEmptyNulTerminatedString) {
    ASSERT_EQ (0U, pstore::utf::length (""));
}
TEST (Utf, LengthOfNonEmptyNullSequence) {
    ASSERT_EQ (0U, pstore::utf::length (nullptr, 1));
}
TEST (Utf, LengthOfNullptr) {
    ASSERT_EQ (0U, pstore::utf::length (nullptr));
}
TEST (Utf, LengthOfSequenceIncludingNullCharacter) {
    static char const str[] = "";
    ASSERT_EQ (1U, pstore::utf::length (str, sizeof (str)));
}



namespace {
    struct SimpleAsciiFixture : public testing::Test {
        static std::string const str;
    };

    std::string const SimpleAsciiFixture::str{"hello mum"};
} // namespace
TEST_F (SimpleAsciiFixture, LengthWithExplicitSize) {
    ASSERT_EQ (9U, pstore::utf::length (str.c_str (), 9));
}
TEST_F (SimpleAsciiFixture, LengthWithNulTerminatedString) {
    ASSERT_EQ (9U, pstore::utf::length (str.c_str ()));
}
TEST_F (SimpleAsciiFixture, IndexCStr) {
    char const * strz = str.c_str ();
    ASSERT_EQ (strz + 0, pstore::utf::index (strz, 0));
    ASSERT_EQ (strz + 1, pstore::utf::index (strz, 1));
    ASSERT_EQ (strz + 2, pstore::utf::index (strz, 2));
    ASSERT_EQ (strz + 3, pstore::utf::index (strz, 3));
    ASSERT_EQ (strz + 4, pstore::utf::index (strz, 4));
    ASSERT_EQ (strz + 5, pstore::utf::index (strz, 5));
    ASSERT_EQ (strz + 6, pstore::utf::index (strz, 6));
    ASSERT_EQ (strz + 7, pstore::utf::index (strz, 7));
    ASSERT_EQ (strz + 8, pstore::utf::index (strz, 8));
    ASSERT_EQ (nullptr, pstore::utf::index (strz, 9));
    ASSERT_EQ (nullptr, pstore::utf::index (strz, std::numeric_limits<std::size_t>::max ()));
}
TEST_F (SimpleAsciiFixture, IndexStdString) {
    std::string::const_iterator begin = std::begin (str);
    std::string::const_iterator end = std::end (str);
    auto it = begin;
    ASSERT_EQ (it, pstore::utf::index (begin, end, 0));
    std::advance (it, 1);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 1));
    std::advance (it, 1);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 2));
    std::advance (it, 1);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 3));
    std::advance (it, 1);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 4));
    std::advance (it, 1);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 5));
    std::advance (it, 1);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 6));
    std::advance (it, 1);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 7));
    std::advance (it, 1);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 8));

    ASSERT_EQ (end, pstore::utf::index (begin, end, 9));
    ASSERT_EQ (end, pstore::utf::index (begin, end, std::numeric_limits<std::size_t>::max ()));
}


namespace {
    struct ShortJapaneseStringFixture : public testing::Test {
        static std::uint8_t const bytes[];
        static std::string const str;
    };

    std::uint8_t const ShortJapaneseStringFixture::bytes[] = {
        0xE3, 0x81, 0x8A, // HIRAGANA LETTER O
        0xE3, 0x81, 0xAF, // HIRAGANA LETTER HA
        0xE3, 0x82, 0x88, // HIRAGANA LETTER YO
        0xE3, 0x81, 0x86, // HIRAGANA LETTER U
        0xE3, 0x81, 0x94, // HIRAGANA LETTER GO
        0xE3, 0x81, 0x96, // HIRAGANA LETTER ZA
        0xE3, 0x81, 0x84, // HIRAGANA LETTER I
        0xE3, 0x81, 0xBE, // HIRAGANA LETTER MA
        0xE3, 0x81, 0x99, // HIRAGANA LETTER SU
        0x00,             // NULL
    };
    std::string const ShortJapaneseStringFixture::str{reinterpret_cast<char const *> (bytes)};
} // namespace

TEST_F (ShortJapaneseStringFixture, LengthWithExplicitSize) {
    ASSERT_EQ (9U, pstore::utf::length (str.c_str (), sizeof (bytes) - 1));
}
TEST_F (ShortJapaneseStringFixture, LengthWithNulTerminatedString) {
    ASSERT_EQ (9U, pstore::utf::length (str.c_str ()));
}
TEST_F (ShortJapaneseStringFixture, IndexCstr) {
    char const * strz = str.c_str ();
    ASSERT_EQ (strz + 0, pstore::utf::index (strz, 0));
    ASSERT_EQ (strz + 3, pstore::utf::index (strz, 1));
    ASSERT_EQ (strz + 6, pstore::utf::index (strz, 2));
    ASSERT_EQ (strz + 9, pstore::utf::index (strz, 3));
    ASSERT_EQ (strz + 12, pstore::utf::index (strz, 4));
    ASSERT_EQ (strz + 15, pstore::utf::index (strz, 5));
    ASSERT_EQ (strz + 18, pstore::utf::index (strz, 6));
    ASSERT_EQ (strz + 21, pstore::utf::index (strz, 7));
    ASSERT_EQ (strz + 24, pstore::utf::index (strz, 8));
    ASSERT_EQ (static_cast<char const *> (nullptr), pstore::utf::index (strz, 9));
}

TEST_F (ShortJapaneseStringFixture, IndexStdString) {
    std::string::const_iterator begin = std::begin (str);
    std::string::const_iterator end = std::end (str);

    auto it = begin;
    ASSERT_EQ (it, pstore::utf::index (begin, end, 0));
    std::advance (it, 3);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 1));
    std::advance (it, 3);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 2));
    std::advance (it, 3);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 3));
    std::advance (it, 3);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 4));
    std::advance (it, 3);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 5));
    std::advance (it, 3);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 6));
    std::advance (it, 3);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 7));
    std::advance (it, 3);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 8));

    ASSERT_EQ (end, pstore::utf::index (begin, end, 9));
}


namespace {
    struct FourByteUtf8ChineseCharacters : public testing::Test {
        static std::uint8_t const bytes[];
        static std::string const str;
    };

    std::uint8_t const FourByteUtf8ChineseCharacters::bytes[] = {
        0xF0, 0xA0, 0x9C, 0x8E, // CJK UNIFIED IDEOGRAPH-2070E
        0xF0, 0xA0, 0x9C, 0xB1, // CJK UNIFIED IDEOGRAPH-20731
        0xF0, 0xA0, 0x9D, 0xB9, // CJK UNIFIED IDEOGRAPH-20779
        0xF0, 0xA0, 0xB1, 0x93, // CJK UNIFIED IDEOGRAPH-20C53
        0x00,                   // NULL
    };
    std::string const FourByteUtf8ChineseCharacters::str{reinterpret_cast<char const *> (bytes)};
} // namespace

TEST_F (FourByteUtf8ChineseCharacters, LengthWithExplicitSize) {
    ASSERT_EQ (4U, pstore::utf::length (str.c_str (), sizeof (bytes) - 1));
}
TEST_F (FourByteUtf8ChineseCharacters, LengthWithNulTerminatedString) {
    ASSERT_EQ (4U, pstore::utf::length (str.c_str (), sizeof (bytes) - 1));
}
TEST_F (FourByteUtf8ChineseCharacters, IndexCstr) {
    char const * strz = str.c_str ();
    ASSERT_EQ (strz + 0, pstore::utf::index (strz, 0));
    ASSERT_EQ (strz + 4, pstore::utf::index (strz, 1));
    ASSERT_EQ (strz + 8, pstore::utf::index (strz, 2));
    ASSERT_EQ (strz + 12, pstore::utf::index (strz, 3));
    ASSERT_EQ (static_cast<char const *> (nullptr), pstore::utf::index (strz, 4));
}
TEST_F (FourByteUtf8ChineseCharacters, IndexStdString) {
    std::string::const_iterator begin = std::begin (str);
    std::string::const_iterator end = std::end (str);

    auto it = begin;
    ASSERT_EQ (it, pstore::utf::index (begin, end, 0));
    std::advance (it, 4);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 1));
    std::advance (it, 4);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 2));
    std::advance (it, 4);
    ASSERT_EQ (it, pstore::utf::index (begin, end, 3));

    ASSERT_EQ (end, pstore::utf::index (begin, end, 4));
}


namespace {
    // Test the highest Unicode value that is still resulting in an
    // overlong sequence if represented with the given number of bytes. This
    // is a boundary test for safe UTF-8 decoders. All four characters should
    // be rejected like malformed UTF-8 sequences.
    // Since IETF RFC 3629 modified the UTF-8 definition, any encodings beyond
    // 4 bytes are now illegal
    // (see http://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html)

    struct MaxLengthUTFSequence : public testing::Test {
        static std::uint8_t const bytes[];
        static char const * str;
    };

    std::uint8_t const MaxLengthUTFSequence::bytes[] = {
        0x7F,                   // U+007F DELETE
        0xDF, 0xBF,             // U+07FF
        0xEF, 0xBF, 0xBF,       // U+FFFF
        0xF4, 0x8F, 0xBF, 0xBF, // U+10FFFF
        0x00,                   // U+0000 NULL
    };

    char const * MaxLengthUTFSequence::str = reinterpret_cast<char const *> (bytes);
} // namespace

TEST_F (MaxLengthUTFSequence, LengthWithExplicitSize) {
    ASSERT_EQ (4U, pstore::utf::length (str, sizeof (bytes) - 1));
}
TEST_F (MaxLengthUTFSequence, LengthWithNulTerminatedString) {
    ASSERT_EQ (4U, pstore::utf::length (str));
}
TEST_F (MaxLengthUTFSequence, Index) {
    ASSERT_EQ (str + 0, pstore::utf::index (str, 0));
    ASSERT_EQ (str + 1, pstore::utf::index (str, 1));
    ASSERT_EQ (str + 3, pstore::utf::index (str, 2));
    ASSERT_EQ (str + 6, pstore::utf::index (str, 3));
    ASSERT_EQ (static_cast<char const *> (nullptr), pstore::utf::index (str, 4));
}

TEST_F (MaxLengthUTFSequence, Slice) {
    std::ptrdiff_t start = 0;
    std::ptrdiff_t end = 1;
    std::tie (start, end) = pstore::utf::slice (str, start, end);
    ASSERT_EQ (0, start);
    ASSERT_EQ (1, end);

    start = 0;
    end = 2;
    std::tie (start, end) = pstore::utf::slice (str, start, end);
    ASSERT_EQ (0, start);
    ASSERT_EQ (3, end);

    start = 0;
    end = 3;
    std::tie (start, end) = pstore::utf::slice (str, start, end);
    ASSERT_EQ (0, start);
    ASSERT_EQ (6, end);

    start = 0;
    end = 4;
    std::tie (start, end) = pstore::utf::slice (str, start, end);
    ASSERT_EQ (0, start);
    ASSERT_EQ (-1, end);

    start = 3;
    end = 3;
    std::tie (start, end) = pstore::utf::slice (str, start, end);
    ASSERT_EQ (6, start);
    ASSERT_EQ (6, end);
}
// eof: unittests/pstore_support/test_utf.cpp
