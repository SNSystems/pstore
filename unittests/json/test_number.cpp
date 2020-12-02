//*                        _                *
//*  _ __  _   _ _ __ ___ | |__   ___ _ __  *
//* | '_ \| | | | '_ ` _ \| '_ \ / _ \ '__| *
//* | | | | |_| | | | | | | |_) |  __/ |    *
//* |_| |_|\__,_|_| |_| |_|_.__/ \___|_|    *
//*                                         *
//===- unittests/json/test_number.cpp -------------------------------------===//
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

#include <gtest/gtest.h>

#include "pstore/json/dom_types.hpp"
#include "callbacks.hpp"

using namespace std::string_literals;
using namespace pstore;
using testing::DoubleEq;
using testing::StrictMock;

namespace {

    class JsonNumber : public ::testing::Test {
    public:
        JsonNumber ()
                : proxy_{callbacks_} {}

    protected:
        StrictMock<mock_json_callbacks> callbacks_;
        callbacks_proxy<mock_json_callbacks> proxy_;
    };

} // end of anonymous namespace

TEST_F (JsonNumber, Zero) {
    EXPECT_CALL (callbacks_, uint64_value (0)).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("0"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, NegativeZero) {
    EXPECT_CALL (callbacks_, int64_value (0)).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-0"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, One) {
    EXPECT_CALL (callbacks_, uint64_value (1)).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input (" 1 "s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, LeadingZero) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("01"s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, MinusOne) {
    EXPECT_CALL (callbacks_, int64_value (-1)).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-1"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, MinusOneLeadingZero) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-01"s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, MinusOnly) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-"s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::expected_digits));
}
TEST_F (JsonNumber, MinusMinus) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("--"s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::unrecognized_token));
}

TEST_F (JsonNumber, AllDigits) {
    EXPECT_CALL (callbacks_, uint64_value (1234567890UL)).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1234567890"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, PositivePi) {
    EXPECT_CALL (callbacks_, double_value (DoubleEq (3.1415))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("3.1415"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, NegativePi) {
    EXPECT_CALL (callbacks_, double_value (DoubleEq (-3.1415))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-3.1415"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, PositiveZeroPoint45) {
    EXPECT_CALL (callbacks_, double_value (DoubleEq (0.45))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("0.45"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, NegativeZeroPoint45) {
    EXPECT_CALL (callbacks_, double_value (DoubleEq (-0.45))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-0.45"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, ZeroExp2) {
    EXPECT_CALL (callbacks_, double_value (DoubleEq (0))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("0e2"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, OneExp2) {
    EXPECT_CALL (callbacks_, double_value (DoubleEq (100.0))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1e2"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, OneExpPlus2) {
    EXPECT_CALL (callbacks_, double_value (DoubleEq (100.0))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1e+2"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, ZeroPointZeroOne) {
    EXPECT_CALL (callbacks_, double_value (DoubleEq (0.01))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("0.01"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, OneExpMinus2) {
    EXPECT_CALL (callbacks_, double_value (DoubleEq (0.01))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1e-2"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, OneCapitalExpMinus2) {
    EXPECT_CALL (callbacks_, double_value (DoubleEq (0.01))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1E-2"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, OneExpMinusZero2) {
    EXPECT_CALL (callbacks_, double_value (DoubleEq (0.01))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1E-02"s).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, IntegerMax) {
    constexpr auto long_max = std::numeric_limits<std::int64_t>::max ();
    auto const str_max = std::to_string (long_max);

    EXPECT_CALL (callbacks_, uint64_value (long_max)).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input (str_max).eof ();
    EXPECT_FALSE (p.has_error ());
}

namespace {

    constexpr auto uint64_max = UINT64_C(18446744073709551615);
    static_assert (uint64_max == std::numeric_limits<std::uint64_t>::max (),
                   "Hard-wired unsigned 64-bit max value seems to be incorrect");
    constexpr auto uint64_max_str = "18446744073709551615"; // string equivalent of uint64_max.
    constexpr auto uint64_overflow = "18446744073709551616"; // uint64_max plus 1.

// FIXME: talk about C++!
    // The literal "most negative int" cannot be written in C. The rules in the standard (section
    //  6.4.4.1 in C99) will give it an unsigned type.
    constexpr auto int64_min = std::numeric_limits<std::int64_t>::min ();
    constexpr auto int64_min_str = "-9223372036854775808";
    constexpr auto int64_overflow = "-9223372036854775809"; // int64_min minus 1.

} // end anonymous namespace


TEST_F (JsonNumber, Uint64Max) {
    assert (uint64_max_str == std::to_string (uint64_max) &&
            "The hard-wired unsigned 64-bit max string seems to be incorrect");
    EXPECT_CALL (callbacks_, uint64_value (uint64_max)).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input (std::string{uint64_max_str}).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, Int64Min) {
    assert (int64_min_str == std::to_string (int64_min) &&
           "The hard-wired signed 64-bit min string seems to be incorrect");
    EXPECT_CALL (callbacks_, int64_value (int64_min)).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input (std::string{int64_min_str}).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, IntegerPositiveOverflow) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input (std::string{uint64_overflow}).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, IntegerNegativeOverflow1) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-123123123123123123123123123123"s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, IntegerNegativeOverflow2) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input (std::string{int64_overflow}).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, RealPositiveOverflow) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("123123e100000"s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, RealPositiveOverflow2) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("9999E999"s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, RealUnderflow) {
    json::parser<decltype (proxy_)> p = json::make_parser (proxy_);
    p.input ("123e-10000000"s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, BadExponentDigit) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1Ex"s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::unrecognized_token));
}

TEST_F (JsonNumber, BadFractionDigit) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1.."s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::unrecognized_token));
}
TEST_F (JsonNumber, BadExponentAfterPoint) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1.E"s).eof ();
    EXPECT_EQ (p.last_error (), make_error_code (json::error_code::unrecognized_token));
}
