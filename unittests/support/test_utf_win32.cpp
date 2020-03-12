//*        _    __  *
//*  _   _| |_ / _| *
//* | | | | __| |_  *
//* | |_| | |_|  _| *
//*  \__,_|\__|_|   *
//*                 *
//===- unittests/support/test_utf_win32.cpp -------------------------------===//
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
/// \file test_utf_win32.cpp
/// \brief A small number of tests for the Win32-only utf-16 <-> utf-8 conversion utilities.

#ifdef _WIN32

// 3rd party includes
#    include "gtest/gtest.h"

// pstore includes
#    include "pstore/support/utf.hpp"

namespace {
    struct UtfStringsFixture : testing::Test {
        static std::uint8_t const utf8_bytes[];
        static char const * utf8_str;

        static wchar_t const utf16_str[];
    };

    std::uint8_t const UtfStringsFixture::utf8_bytes[] = {
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
    char const * UtfStringsFixture::utf8_str = reinterpret_cast<char const *> (utf8_bytes);

    wchar_t const UtfStringsFixture::utf16_str[] = {
        0x304A, // HIRAGANA LETTER O
        0x306F, // HIRAGANA LETTER HA
        0x3088, // HIRAGANA LETTER YO
        0x3046, // HIRAGANA LETTER U
        0x3054, // HIRAGANA LETTER GO
        0x3056, // HIRAGANA LETTER ZA
        0x3044, // HIRAGANA LETTER I
        0x307E, // HIRAGANA LETTER MA
        0x3059, // HIRAGANA LETTER SU
        0x0000, // NULL
    };
} // namespace

TEST_F (UtfStringsFixture, Utf8To16Empty) {
    std::wstring const reply = pstore::utf::win32::to16 ("");
    EXPECT_EQ (std::wstring{}, reply);
}

TEST_F (UtfStringsFixture, Utf8To16) {
    std::wstring const reply = pstore::utf::win32::to16 (utf8_str);
    EXPECT_EQ (std::wstring{utf16_str}, reply);
}

TEST_F (UtfStringsFixture, Utf8To16StdString) {
    std::string const input{utf8_str};
    std::wstring const reply = pstore::utf::win32::to16 (input);
    EXPECT_EQ (std::wstring{utf16_str}, reply);
}

TEST_F (UtfStringsFixture, Utf16To8Empty) {
    std::string const reply = pstore::utf::win32::to8 (std::wstring{});
    EXPECT_EQ (std::string{}, reply);
}

TEST_F (UtfStringsFixture, Utf16To8) {
    std::string const reply = pstore::utf::win32::to8 (utf16_str);
    EXPECT_EQ (std::string (utf8_str), reply);
}


TEST (UtfStrings, BadUtf16Input) {
    // From the Unicode FAQ:
    // "Unpaired surrogates are invalid in UTFs. These include any value in
    // the range D80016 to DBFF16 not followed by a value in the range DC0016
    // to DFFF16, or any value in the range DC0016 to DFFF16 not preceded by
    // a value in the range D80016 to DBFF16."

    std::wstring const bad{
        0xD800, // first character of surrogate pair (second pair missing!)
        0x0041, // LATIN CAPITAL LETTER A
    };
    std::uint8_t const expected_bytes[]{
        0xEF, 0xBF, 0xBD, // REPLACEMENT CHARACTER U+FFFD
        0x41,             // LATIN CAPITAL LETTER A
        0x00,             // NULL
    };

    std::string const reply = pstore::utf::win32::to8 (bad);
    EXPECT_EQ (reinterpret_cast<char const *> (expected_bytes), reply);
}

TEST (UtfStrings, BadUtf8Input) {
    // From the Unicode FAQ:
    // "Unpaired surrogates are invalid in UTFs. These include any value in
    // the range D80016 to DBFF16 not followed by a value in the range DC0016
    // to DFFF16, or any value in the range DC0016 to DFFF16 not preceded by
    // a value in the range D80016 to DBFF16."

    std::uint8_t const bad[]{
        0xFE,
        0x41, // LATIN CAPITAL A
        0x00, // NULL
    };
    std::wstring const expected{
        0xFFFD, // REPLACEMENT CHARACTER U+FFFD
        0x0041, // LATIN CAPITAL LETTER A
    };
    std::wstring const reply =
        pstore::utf::win32::to16 (reinterpret_cast<pstore::gsl::czstring> (bad));
    EXPECT_EQ (expected, reply);
}

#endif // _WIN32
