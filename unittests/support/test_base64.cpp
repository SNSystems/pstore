//*  _                     __   _  _    *
//* | |__   __ _ ___  ___ / /_ | || |   *
//* | '_ \ / _` / __|/ _ \ '_ \| || |_  *
//* | |_) | (_| \__ \  __/ (_) |__   _| *
//* |_.__/ \__,_|___/\___|\___/   |_|   *
//*                                     *
//===- unittests/support/test_base64.cpp ----------------------------------===//
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

/// \file test_base64.cpp

#include "pstore/support/base64.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace {

    class Base64Fixture : public ::testing::Test {
    public:
        template <typename InputIterator>
        std::string convert (InputIterator begin, InputIterator end) const;
        std::string convert (std::string const & source) const;
    };

    template <typename InputIterator>
    std::string Base64Fixture::convert (InputIterator begin, InputIterator end) const {
        std::ostringstream out;
        pstore::to_base64 (begin, end, std::ostream_iterator<char> (out, ""));
        return out.str ();
    }

    std::string Base64Fixture::convert (std::string const & source) const {
        return this->convert (std::begin (source), std::end (source));
    }

} // namespace

// Test vectors from RFC 4648:
//
//   BASE64("f") = "Zg=="
//   BASE64("fo") = "Zm8="
//   BASE64("foo") = "Zm9v"
//   BASE64("foob") = "Zm9vYg=="
//   BASE64("fooba") = "Zm9vYmE="
//   BASE64("foobar") = "Zm9vYmFy"

TEST_F (Base64Fixture, RFC4648Empty) {
    std::string const & actual = convert ("");
    EXPECT_EQ ("", actual);
}

TEST_F (Base64Fixture, RFC4648OneChar) {
    std::string const & actual = convert ("f");
    EXPECT_EQ ("Zg==", actual);
}

TEST_F (Base64Fixture, RFC4648TwoChars) {
    std::string const & actual = convert ("fo");
    EXPECT_EQ ("Zm8=", actual);
}

TEST_F (Base64Fixture, RFC4648ThreeChars) {
    std::string const & actual = convert ("foo");
    EXPECT_EQ ("Zm9v", actual);
}

TEST_F (Base64Fixture, RFC4648FourChars) {
    std::string const & actual = convert ("foob");
    EXPECT_EQ ("Zm9vYg==", actual);
}

TEST_F (Base64Fixture, RFC4648FiveChars) {
    std::string const & actual = convert ("fooba");
    EXPECT_EQ ("Zm9vYmE=", actual);
}

TEST_F (Base64Fixture, RFC4648SixChars) {
    std::string const & actual = convert ("foobar");
    EXPECT_EQ ("Zm9vYmFy", actual);
}

TEST_F (Base64Fixture, LongInput) {
    std::vector<std::uint8_t> input;
    input.reserve (256);
    auto value = std::uint8_t{0};
    std::generate_n (std::back_inserter (input), 256, [&value] () { return value++; });

    std::string const & actual = convert (std::begin (input), std::end (input));

    std::string const expected =
        "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7"
        "PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3"
        "eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKz"
        "tLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v"
        "8PHy8/T19vf4+fr7/P3+/w==";

    EXPECT_EQ (expected, actual);
}
