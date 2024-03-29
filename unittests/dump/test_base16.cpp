//===- unittests/dump/test_base16.cpp -------------------------------------===//
//*  _                    _  __    *
//* | |__   __ _ ___  ___/ |/ /_   *
//* | '_ \ / _` / __|/ _ \ | '_ \  *
//* | |_) | (_| \__ \  __/ | (_) | *
//* |_.__/ \__,_|___/\___|_|\___/  *
//*                                *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/// \file test_base16.cpp

#include "pstore/dump/value.hpp"

// Standard library includes
#include <cstdint>
#include <vector>

// 3rd party includes
#include <gtest/gtest.h>

namespace {
    class Base16Fixture : public ::testing::Test {
    public:
        template <typename InputIterator>
        std::string convert (InputIterator begin, InputIterator end) const;
        std::string convert (std::string const & source) const;
        static std::string const prefix;
    };

    template <typename InputIterator>
    std::string Base16Fixture::convert (InputIterator begin, InputIterator end) const {
        using namespace ::pstore::dump;
        binary16 b (begin, end);
        std::ostringstream out;
        b.write (out);
        return out.str ();
    }

    std::string Base16Fixture::convert (std::string const & source) const {
        return this->convert (std::begin (source), std::end (source));
    }

    std::string const Base16Fixture::prefix = "!!binary16 |\n";
} // namespace

TEST_F (Base16Fixture, RFC4648Empty) {
    std::string const & actual = convert ("");
    EXPECT_EQ (prefix + '>', actual);
}

TEST_F (Base16Fixture, RFC4648OneChar) {
    std::string const & actual = convert ("f");
    EXPECT_EQ (prefix + "66>", actual);
}

TEST_F (Base16Fixture, RFC4648TwoChars) {
    std::string const & actual = convert ("fo");
    EXPECT_EQ (prefix + "666F>", actual);
}

TEST_F (Base16Fixture, RFC4648ThreeChars) {
    std::string const & actual = convert ("foo");
    EXPECT_EQ (prefix + "666F 6F>", actual);
}

TEST_F (Base16Fixture, RFC4648FourChars) {
    std::string const & actual = convert ("foob");
    EXPECT_EQ (prefix + "666F 6F62>", actual);
}

TEST_F (Base16Fixture, RFC4648FiveChars) {
    std::string const & actual = convert ("fooba");
    EXPECT_EQ (prefix + "666F 6F62 61>", actual);
}

TEST_F (Base16Fixture, RFC4648SixChars) {
    std::string const & actual = convert ("foobar");
    EXPECT_EQ (prefix + "666F 6F62 6172>", actual);
}

namespace {
    template <typename OutputIterator, typename Size, typename Assignable>
    void iota_n (OutputIterator first, Size n, Assignable value) {
        std::generate_n (first, n, [&value] () { return value++; });
    }
} // namespace

TEST_F (Base16Fixture, LongInput) {
    std::vector<std::uint8_t> input;
    input.reserve (256);
    iota_n (std::back_inserter (input), std::size_t{256}, std::uint8_t{0});

    std::string const & actual = convert (std::begin (input), std::end (input));
    std::string const expected = prefix + "0001 0203 0405 0607 0809 0A0B 0C0D 0E0F\n"
                                          "1011 1213 1415 1617 1819 1A1B 1C1D 1E1F\n"
                                          "2021 2223 2425 2627 2829 2A2B 2C2D 2E2F\n"
                                          "3031 3233 3435 3637 3839 3A3B 3C3D 3E3F\n"
                                          "4041 4243 4445 4647 4849 4A4B 4C4D 4E4F\n"
                                          "5051 5253 5455 5657 5859 5A5B 5C5D 5E5F\n"
                                          "6061 6263 6465 6667 6869 6A6B 6C6D 6E6F\n"
                                          "7071 7273 7475 7677 7879 7A7B 7C7D 7E7F\n"
                                          "8081 8283 8485 8687 8889 8A8B 8C8D 8E8F\n"
                                          "9091 9293 9495 9697 9899 9A9B 9C9D 9E9F\n"
                                          "A0A1 A2A3 A4A5 A6A7 A8A9 AAAB ACAD AEAF\n"
                                          "B0B1 B2B3 B4B5 B6B7 B8B9 BABB BCBD BEBF\n"
                                          "C0C1 C2C3 C4C5 C6C7 C8C9 CACB CCCD CECF\n"
                                          "D0D1 D2D3 D4D5 D6D7 D8D9 DADB DCDD DEDF\n"
                                          "E0E1 E2E3 E4E5 E6E7 E8E9 EAEB ECED EEEF\n"
                                          "F0F1 F2F3 F4F5 F6F7 F8F9 FAFB FCFD FEFF>";
    EXPECT_EQ (expected, actual);
}
