//*  _            __  __                   _                      _            *
//* | |__  _   _ / _|/ _| ___ _ __ ___  __| |  _ __ ___  __ _  __| | ___ _ __  *
//* | '_ \| | | | |_| |_ / _ \ '__/ _ \/ _` | | '__/ _ \/ _` |/ _` |/ _ \ '__| *
//* | |_) | |_| |  _|  _|  __/ | |  __/ (_| | | | |  __/ (_| | (_| |  __/ |    *
//* |_.__/ \__,_|_| |_|  \___|_|  \___|\__,_| |_|  \___|\__,_|\__,_|\___|_|    *
//*                                                                            *
//===- unittests/httpd/test_buffered_reader.cpp ---------------------------===//
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
#include "pstore/httpd/buffered_reader.hpp"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>

#include "gmock/gmock.h"

#include "buffered_reader_mocks.hpp"

using pstore::error_or;
using pstore::error_or_n;
using pstore::in_place;
using pstore::maybe;
using pstore::gsl::span;
using pstore::httpd::make_buffered_reader;
using testing::_;
using testing::Invoke;

TEST (HttpdBufferedReader, Span) {
    using byte_span = pstore::gsl::span<std::uint8_t>;
    auto refill = [](int io, byte_span const & sp) {
        std::memset (sp.data (), 0, sp.size ());
        return pstore::error_or_n<int, byte_span::iterator>{pstore::in_place, io, sp.end ()};
    };

    constexpr auto buffer_size = std::size_t{0};
    constexpr auto requested_size = std::size_t{1};
    auto br = pstore::httpd::make_buffered_reader<int> (refill, buffer_size);

    std::vector<std::uint8_t> v (requested_size, std::uint8_t{0xFF});
    pstore::error_or_n<int, byte_span> res = br.get_span (0, pstore::gsl::make_span (v));
    ASSERT_TRUE (res);
    auto const & sp = std::get<1> (res);
    ASSERT_EQ (sp.size (), 1);
    EXPECT_EQ (sp[0], std::uint8_t{0});
}

TEST (HttpdBufferedReader, GetcThenEOF) {
    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));
    EXPECT_CALL (r, fill (0, _)).WillOnce (Invoke (yield_string ("a")));

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());
    {
        getc_result_type const c1 = br.getc (io);
        ASSERT_TRUE (static_cast<bool> (c1));
        io = std::get<0> (*c1);
        maybe<char> const char1 = std::get<1> (*c1);
        ASSERT_TRUE (char1.has_value ());
        EXPECT_EQ (char1.value (), 'a');
    }
    {
        getc_result_type const c2 = br.getc (io);
        ASSERT_TRUE (static_cast<bool> (c2));
        io = std::get<0> (*c2);
        maybe<char> const char2 = std::get<1> (*c2);
        ASSERT_FALSE (char2.has_value ());
    }
}

TEST (HttpdBufferedReader, GetTwoStringsLFThenEOF) {
    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));
    EXPECT_CALL (r, fill (0, _)).WillOnce (Invoke (yield_string ("abc\ndef")));

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());
    {
        gets_result_type const s1 = br.gets (io);
        ASSERT_TRUE (static_cast<bool> (s1));
        maybe<std::string> str1;
        std::tie (io, str1) = *s1;
        ASSERT_TRUE (str1.has_value ());
        EXPECT_EQ (str1.value (), "abc");
    }
    {
        gets_result_type const s2 = br.gets (io);
        ASSERT_TRUE (static_cast<bool> (s2));
        maybe<std::string> str2;
        std::tie (io, str2) = *s2;
        ASSERT_TRUE (str2.has_value ());
        EXPECT_EQ (str2.value (), "def");
    }
    {
        gets_result_type const s3 = br.gets (io);
        ASSERT_TRUE (static_cast<bool> (s3));
        maybe<std::string> str3;
        std::tie (io, str3) = *s3;
        ASSERT_FALSE (str3.has_value ());
    }
}

TEST (HttpdBufferedReader, StringCRLF) {
    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));
    EXPECT_CALL (r, fill (0, _)).WillOnce (Invoke (yield_string ("abc\r\ndef")));

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());
    {
        gets_result_type const s1 = br.gets (io);
        ASSERT_TRUE (static_cast<bool> (s1))
            << "There was an unexpected error: " << s1.get_error ();
        maybe<std::string> str1;
        std::tie (io, str1) = *s1;
        ASSERT_TRUE (str1.has_value ());
        EXPECT_EQ (str1.value (), "abc");
    }
    {
        gets_result_type const s2 = br.gets (io);
        ASSERT_TRUE (static_cast<bool> (s2))
            << "There was an unexpected error: " << s2.get_error ();
        maybe<std::string> str2;
        std::tie (io, str2) = *s2;
        ASSERT_TRUE (str2.has_value ());
        EXPECT_EQ (str2.value (), "def");
    }
    {
        gets_result_type const s3 = br.gets (io);
        ASSERT_TRUE (static_cast<bool> (s3))
            << "There was an unexpected error: " << s3.get_error ();
        maybe<std::string> str3;
        std::tie (io, str3) = *s3;
        ASSERT_FALSE (str3.has_value ());
    }
}

TEST (HttpdBufferedReader, StringCRNoLFThenEOF) {
    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));
    EXPECT_CALL (r, fill (0, _)).WillOnce (Invoke (yield_string ("abc\r")));

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());
    {
        gets_result_type const s1 = br.gets (io);
        ASSERT_TRUE (static_cast<bool> (s1))
            << "There was an unexpected error: " << s1.get_error ();
        maybe<std::string> str1;
        std::tie (io, str1) = *s1;
        ASSERT_TRUE (str1.has_value ());
        EXPECT_EQ (str1.value (), "abc");
    }
    {
        gets_result_type const s2 = br.gets (io);
        ASSERT_TRUE (static_cast<bool> (s2))
            << "There was an unexpected error: " << s2.get_error ();
        maybe<std::string> str2;
        std::tie (io, str2) = *s2;
        ASSERT_FALSE (str2.has_value ());
    }
}

TEST (HttpdBufferedReader, StringCRNoLFChars) {
    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));
    EXPECT_CALL (r, fill (0, _)).WillOnce (Invoke (yield_string ("abc\rdef")));

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());
    {
        gets_result_type const s1 = br.gets (io);
        ASSERT_TRUE (static_cast<bool> (s1))
            << "There was an unexpected error: " << s1.get_error ();
        maybe<std::string> str1;
        std::tie (io, str1) = *s1;
        ASSERT_TRUE (str1.has_value ());
        EXPECT_EQ (str1.value (), "abc");
    }
    {
        gets_result_type const s2 = br.gets (io);
        ASSERT_TRUE (static_cast<bool> (s2))
            << "There was an unexpected error: " << s2.get_error ();
        maybe<std::string> str2;
        std::tie (io, str2) = *s2;
        ASSERT_TRUE (str2.has_value ());
        EXPECT_EQ (str2.value (), "def");
    }
}

TEST (HttpdBufferedReader, SomeCharactersThenAnError) {
    refiller r;
    EXPECT_CALL (r, fill (0, _)).WillOnce (Invoke (yield_string ("abc\nd")));
    EXPECT_CALL (r, fill (1, _)).WillOnce (Invoke ([](int, span<std::uint8_t> const &) {
        return refiller_result_type (std::make_error_code (std::errc::operation_not_permitted));
    }));

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());
    {
        gets_result_type const s1 = br.gets (io);
        ASSERT_TRUE (static_cast<bool> (s1)) << "Error: " << s1.get_error ();
        maybe<std::string> str1;
        std::tie (io, str1) = *s1;
        ASSERT_TRUE (str1.has_value ());
        EXPECT_EQ (str1.value (), "abc");
    }
    {
        gets_result_type const s2 = br.gets (io);
        ASSERT_FALSE (static_cast<bool> (s2)) << "An error was expected";
        EXPECT_EQ (s2.get_error (), (std::make_error_code (std::errc::operation_not_permitted)));
    }
}

TEST (HttpdBufferedReader, MaxLengthString) {
    using pstore::httpd::max_string_length;
    std::string const max_length_string (max_string_length, 'a');

    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));
    EXPECT_CALL (r, fill (0, _)).WillOnce (Invoke (yield_string (max_length_string)));

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function (), max_string_length);
    error_or_n<int, maybe<std::string>> const s1 = br.gets (io);
    ASSERT_TRUE (static_cast<bool> (s1)) << "Error: " << s1.get_error ();
    maybe<std::string> str1;
    std::tie (io, str1) = *s1;
    ASSERT_TRUE (str1.has_value ());
    EXPECT_EQ (str1.value (), max_length_string);
}

TEST (HttpdBufferedReader, StringTooLong) {
    using pstore::httpd::max_string_length;

    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));
    EXPECT_CALL (r, fill (0, _))
        .WillOnce (Invoke (yield_string (std::string (max_string_length + 1U, 'a'))));

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function (), max_string_length + 1U);
    error_or_n<int, maybe<std::string>> const s2 = br.gets (io);
    EXPECT_EQ (s2.get_error (), make_error_code (pstore::httpd::error_code::string_too_long));
}
