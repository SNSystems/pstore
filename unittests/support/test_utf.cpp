//*        _    __  *
//*  _   _| |_ / _| *
//* | | | | __| |_  *
//* | |_| | |_|  _| *
//*  \__,_|\__|_|   *
//*                 *
//===- unittests/support/test_utf.cpp -------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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

#include "pstore/support/utf.hpp"

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

    struct SimpleAsciiFixture : testing::Test {
        static std::string const str;
    };

    std::string const SimpleAsciiFixture::str{"hello mum"};

} // end anonymous namespace

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

    struct ShortJapaneseStringFixture : testing::Test {
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

} // end anonymous namespace

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

    struct FourByteUtf8ChineseCharacters : testing::Test {
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

} // end anonymous namespace

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

    struct MaxLengthUTFSequence : testing::Test {
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


TEST (ByteSwapper, All) {
    EXPECT_EQ (pstore::utf::byte_swapper (0x00FF), 0xFF00);
    EXPECT_EQ (pstore::utf::byte_swapper (0xFF00), 0x00FF);
    EXPECT_EQ (pstore::utf::byte_swapper (0x1234), 0x3412);
}

TEST (CuToUtf8, All) {
    using namespace pstore::utf;

    EXPECT_EQ (code_point_to_utf8<utf8_string> (0x0001), utf8_string ({0x01}));
    EXPECT_EQ (code_point_to_utf8<utf8_string> (0x0024), utf8_string ({0x24}));
    EXPECT_EQ (code_point_to_utf8<utf8_string> (0x00A2), utf8_string ({0xC2, 0xA2}));

    EXPECT_EQ (code_point_to_utf8<utf8_string> (0x007F), utf8_string ({0x7F}));
    EXPECT_EQ (code_point_to_utf8<utf8_string> (0x0080), utf8_string ({0b11000010, 0x80}));
    EXPECT_EQ (code_point_to_utf8<utf8_string> (0x07FF), utf8_string ({0b11011111, 0xBF}));
    EXPECT_EQ (code_point_to_utf8<utf8_string> (0x0800), utf8_string ({0xE0, 0xA0, 0x80}));

    EXPECT_EQ (code_point_to_utf8<utf8_string> (0xD7FF), utf8_string ({0xED, 0x9F, 0xBF}));

    // Since RFC 3629 (November 2003), the high and low surrogate halves used by UTF-16 (U+D800
    // through U+DFFF) and code points not
    // encodable by UTF-16 (those after U+10FFFF) are not legal Unicode values
    EXPECT_EQ (code_point_to_utf8<utf8_string> (0xD800), utf8_string ({0xEF, 0xBF, 0xBD}));
    EXPECT_EQ (code_point_to_utf8<utf8_string> (0xDFFF), utf8_string ({0xEF, 0xBF, 0xBD}));

    EXPECT_EQ (code_point_to_utf8<utf8_string> (0xE000), utf8_string ({0xEE, 0x80, 0x80}));
    EXPECT_EQ (code_point_to_utf8<utf8_string> (0xFFFF), utf8_string ({0xEF, 0xBF, 0xBF}));
    EXPECT_EQ (code_point_to_utf8<utf8_string> (0x10000), utf8_string ({0xF0, 0x90, 0x80, 0x80}));
    EXPECT_EQ (code_point_to_utf8<utf8_string> (0x10FFFF), utf8_string ({0xF4, 0x8F, 0xBF, 0xBF}));
    EXPECT_EQ (code_point_to_utf8<utf8_string> (0x110000), utf8_string ({0xEF, 0xBF, 0xBD}));
}

TEST (Utf16ToUtf8, All) {
    using namespace pstore::utf;

    EXPECT_EQ (utf16_to_code_point (utf16_string ({char16_t{'a'}}), nop_swapper), 97U /*'a'*/);
    EXPECT_EQ (utf16_to_code_point (utf16_string ({char16_t{'a'} << 8}), byte_swapper),
               97U /*'a'*/);
    EXPECT_EQ (utf16_to_code_point (utf16_string ({0x00E0}), byte_swapper), 0xE000U);
    EXPECT_EQ (utf16_to_code_point (utf16_string ({0xD800, 0xDC00}), nop_swapper), 0x010000U);
    EXPECT_EQ (utf16_to_code_point (utf16_string ({0x00D8, 0x00DC}), byte_swapper), 0x010000U);
    EXPECT_EQ (utf16_to_code_point (utf16_string ({0xD800, 0x0000}), nop_swapper),
               replacement_char_code_point);
    EXPECT_EQ (utf16_to_code_point (utf16_string ({0xD800, 0xDBFF}), nop_swapper),
               replacement_char_code_point);
    EXPECT_EQ (utf16_to_code_point (utf16_string ({0xDFFF}), nop_swapper), 0xDFFFU);
}

namespace {
    class Utf8Decode : public ::testing::Test {
    protected:
        using cpstring = std::basic_string<char32_t>;
        using bytes = std::vector<std::uint8_t>;

        cpstring decode (pstore::utf::utf8_decoder & decoder, char const * src) {
            cpstring result;
            for (; *src != '\0'; ++src) {
                if (pstore::maybe<char32_t> const code_point =
                        decoder.get (static_cast<std::uint8_t> (*src))) {
                    result += *code_point;
                }
            }
            return result;
        }

        cpstring decode (bytes && input, bool good) {
            input.push_back (0x00); // add terminating NUL
            pstore::utf::utf8_decoder decoder;
            cpstring result = decode (decoder, reinterpret_cast<char const *> (input.data ()));
            EXPECT_EQ (decoder.is_well_formed (), good);
            return result;
        }
        cpstring decode_good (bytes && input) { return decode (std::move (input), true); }
        cpstring decode_bad (bytes && input) { return decode (std::move (input), false); }
    };
} // namespace

TEST_F (Utf8Decode, Good) {
    std::vector<std::uint8_t> test{
        0xCE, 0xBA, // GREEK SMALL LETTER KAPPA (U+03BA)
        0xCF, 0x8C, // GREEK SMALL LETTER OMICRON WITH TONOS (U+03CC)
        0xCF, 0x83, // GREEK SMALL LETTER SIGMA (U+03C3)
        0xCE, 0xBC, // GREEK SMALL LETTER MU (U+03BC)
        0xCE, 0xB5, // GREEK SMALL LETTER EPSILON (U+03B5)
    };
    EXPECT_EQ (decode_good (std::move (test)), cpstring ({0x03BA, 0x03CC, 0x03C3, 0x03BC, 0x03B5}));
}

TEST_F (Utf8Decode, FirstPossibleSequenceOfACertainLength) {
    EXPECT_EQ (decode_good (bytes ({0x00})),
               cpstring ()); // We treat the NUL character as the end of sequence.
    EXPECT_EQ (decode_good (bytes ({0xC2, 0x80})), cpstring ({0x00000080}));
    EXPECT_EQ (decode_good (bytes ({0xE0, 0xA0, 0x80})), cpstring ({0x00000800}));
    EXPECT_EQ (decode_good (bytes ({0xF0, 0x90, 0x80, 0x80})), cpstring ({0x00010000}));
}
TEST_F (Utf8Decode, LastPossibleSequenceOfACertainLength) {
    EXPECT_EQ (decode_good (bytes ({0x7F})), cpstring ({0x0000007F}));
    EXPECT_EQ (decode_good (bytes ({0xDF, 0xBF})), cpstring ({0x000007FF}));
    EXPECT_EQ (decode_good (bytes ({0xEF, 0xBF, 0xBF})), cpstring ({0x0000FFFF}));
}
TEST_F (Utf8Decode, OtherBoundaryConditions) {
    EXPECT_EQ (decode_good (bytes ({0xED, 0x9F, 0xBF})), cpstring ({0x0000D7FF}));
    EXPECT_EQ (decode_good (bytes ({0xEE, 0x80, 0x80})), cpstring ({0x0000E000}));
    EXPECT_EQ (decode_good (bytes ({0xEF, 0xBF, 0xBD})), cpstring ({0x0000FFFD}));
    EXPECT_EQ (decode_good (bytes ({0xF4, 0x8F, 0xBF, 0xBF})), cpstring ({0x0010FFFF}));
}
TEST_F (Utf8Decode, UnexpectedContinuationBytes) {
    decode_bad (bytes ({0x80}));                   // first continuation byte
    decode_bad (bytes ({0xbf}));                   // last continuation byte
    decode_bad (bytes ({0x80, 0xbf}));             // 2 continuation bytes
    decode_bad (bytes ({0x80, 0xbf, 0x80}));       // 3 continuation bytes
    decode_bad (bytes ({0x80, 0xbf, 0x80, 0xbf})); // 4 continuation bytes
}
TEST_F (Utf8Decode, AllPossibleContinuationBytes) {
    for (std::uint8_t v = 0x80; v <= 0xBF; ++v) {
        decode_bad (bytes ({v})); // first continuation byte
    }
}
TEST_F (Utf8Decode, LonelyStartCharacters) {
    // All 32 first bytes of 2-byte sequences (0xC0-0xDF), each followed by a space character.
    for (std::uint8_t v = 0xC0; v <= 0xDF; ++v) {
        decode_bad (bytes ({v, 0x20}));
    }
    // All 16 first bytes of 3-byte sequences (0xE0-0xEF), each followed by a space character.
    for (std::uint8_t v = 0xE0; v <= 0xEF; ++v) {
        decode_bad (bytes ({v, 0x20}));
    }
    // All 8 first bytes of 4-byte sequences (0xF0-0xF7), each followed by a space character.
    for (std::uint8_t v = 0xF0; v <= 0xF7; ++v) {
        decode_bad (bytes ({v, 0x20}));
    }
}
TEST_F (Utf8Decode, SequencesWithLastContinuationByteMissing) {
    decode_bad (bytes ({0xC0}));             // 2-byte sequence with last byte missing (U+0000)
    decode_bad (bytes ({0xE0, 0x80}));       // 3-byte sequence with last byte missing (U+0000)
    decode_bad (bytes ({0xF0, 0x80, 0x80})); // 4-byte sequence with last byte missing (U+0000)
    decode_bad (bytes ({0xDF}));             // 2-byte sequence with last byte missing (U+000007FF)
    decode_bad (bytes ({0xEF, 0xBF}));       // 3-byte sequence with last byte missing (U-0000FFFF)
    decode_bad (bytes ({0xF7, 0xBF, 0xBF})); // 4-byte sequence with last byte missing (U-001FFFFF)

    decode_bad (bytes ({0xC0, 0xE0, 0x80, 0xF0, 0x80, 0x80, 0xDF, 0xEF, 0xBF, 0xF7, 0xBF, 0xBF}));
}
TEST_F (Utf8Decode, ImpossibleBytes) {
    decode_bad (bytes ({0xFE}));
    decode_bad (bytes ({0xFF}));
    decode_bad (bytes ({0xFE, 0xFE, 0xFF, 0xFF}));
}
TEST_F (Utf8Decode, OverlongAscii) {
    decode_bad (bytes ({0xC0, 0xAF}));             // U+002F
    decode_bad (bytes ({0xE0, 0x80, 0xAF}));       // U+002F
    decode_bad (bytes ({0xF0, 0x80, 0x80, 0xAF})); // U+002F
}
TEST_F (Utf8Decode, MaximumOverlongSequences) {
    decode_bad (bytes ({0xC1, 0xBF}));             // U-0000007F
    decode_bad (bytes ({0xE0, 0x9F, 0xBF}));       // U-000007FF
    decode_bad (bytes ({0xF0, 0x8F, 0xBF, 0xBF})); // U-0000FFFF
}
TEST_F (Utf8Decode, OverlongNul) {
    decode_bad (bytes ({0xC0, 0x80}));             // U+0000
    decode_bad (bytes ({0xE0, 0x80, 0x80}));       // U+0000
    decode_bad (bytes ({0xF0, 0x80, 0x80, 0x80})); // U+0000
}
TEST_F (Utf8Decode, IllegalCodePositions) {
    // Single UTF-16 surrogates
    decode_bad (bytes ({0xED, 0xA0, 0x80})); // U+D800
    decode_bad (bytes ({0xED, 0xAD, 0xBF})); // U+DB7F
    decode_bad (bytes ({0xED, 0xAE, 0x80})); // U+DB80
    decode_bad (bytes ({0xED, 0xAF, 0xBF})); // U+DBFF
    decode_bad (bytes ({0xED, 0xB0, 0x80})); // U+DC00
    decode_bad (bytes ({0xED, 0xBE, 0x80})); // U+DF80
    decode_bad (bytes ({0xED, 0xBF, 0xBF})); // U+DFFF

    // Paired UTF-16 surrogates
    decode_bad (bytes ({0xED, 0xA0, 0x80, 0xED, 0xB0, 0x80})); // U+D800 U+DC00
    decode_bad (bytes ({0xED, 0xA0, 0x80, 0xED, 0xBF, 0xBF})); // U+D800 U+DFFF
    decode_bad (bytes ({0xED, 0xAD, 0xBF, 0xED, 0xB0, 0x80})); // U+DB7F U+DC00
    decode_bad (bytes ({0xED, 0xAD, 0xBF, 0xED, 0xBF, 0xBF})); // U+DB7F U+DFFF
    decode_bad (bytes ({0xED, 0xAE, 0x80, 0xED, 0xB0, 0x80})); // U+DB80 U+DC00
    decode_bad (bytes ({0xED, 0xAE, 0x80, 0xED, 0xBF, 0xBF})); // U+DB80 U+DFFF
    decode_bad (bytes ({0xED, 0xAF, 0xBF, 0xED, 0xB0, 0x80})); // U+DBFF U+DC00
    decode_bad (bytes ({0xED, 0xAF, 0xBF, 0xED, 0xBF, 0xBF})); // U+DBFF U+DFFF
}
