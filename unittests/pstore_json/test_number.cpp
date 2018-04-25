//*                        _                *
//*  _ __  _   _ _ __ ___ | |__   ___ _ __  *
//* | '_ \| | | | '_ ` _ \| '_ \ / _ \ '__| *
//* | | | | |_| | | | | | | |_) |  __/ |    *
//* |_| |_|\__,_|_| |_| |_|_.__/ \___|_|    *
//*                                         *
//===- unittests/pstore_json/test_number.cpp ------------------------------===//
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
#include <gtest/gtest.h>
#include "pstore/json/dom_types.hpp"
#include "callbacks.hpp"

using namespace pstore;
using testing::StrictMock;
using testing::DoubleEq;

namespace {

class JsonNumber : public ::testing::Test {
public:
    JsonNumber () : proxy_{callbacks_} {}
protected:
    StrictMock<mock_json_callbacks> callbacks_;
    callbacks_proxy<mock_json_callbacks> proxy_;
};

}  //end of anonymous namespace

TEST_F (JsonNumber, Zero) {
    EXPECT_CALL (callbacks_, integer_value (0L)).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("0").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, NegativeZero) {
    EXPECT_CALL (callbacks_, integer_value (0L)).Times (1);

    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-0").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, One) {
    EXPECT_CALL (callbacks_, integer_value (1L)).Times (1);

    json::parser<decltype (proxy_)> p (proxy_);
    p.input (" 1 ").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, LeadingZero) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("01").eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, MinusOne) {
    EXPECT_CALL (callbacks_, integer_value (-1)).Times (1);

    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-1").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, MinusOneLeadingZero) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-01").eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, MinusOnly) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-").eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::expected_digits));
}
TEST_F (JsonNumber, MinusMinus) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("--").eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
}

TEST_F (JsonNumber, AllDigits) {
    EXPECT_CALL (callbacks_, integer_value (1234567890L)).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1234567890").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, PositivePi) {
    EXPECT_CALL (callbacks_, float_value (DoubleEq (3.1415))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("3.1415").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, NegativePi) {
    EXPECT_CALL (callbacks_, float_value (DoubleEq (-3.1415))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-3.1415").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, PositiveZeroPoint45) {
    EXPECT_CALL (callbacks_, float_value (DoubleEq (0.45))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("0.45").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, NegativeZeroPoint45) {
    EXPECT_CALL (callbacks_, float_value (DoubleEq (-0.45))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-0.45").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, ZeroExp2) {
    EXPECT_CALL (callbacks_, float_value (DoubleEq (0))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("0e2").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, OneExp2) {
    EXPECT_CALL (callbacks_, float_value (DoubleEq (100.0))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1e2").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, OneExpPlus2) {
    EXPECT_CALL (callbacks_, float_value (DoubleEq (100.0))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1e+2").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, ZeroPointZeroOne) {
    EXPECT_CALL (callbacks_, float_value (DoubleEq (0.01))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("0.01").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, OneExpMinus2) {
    EXPECT_CALL (callbacks_, float_value (DoubleEq (0.01))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1e-2").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, OneCapitalExpMinus2) {
    EXPECT_CALL (callbacks_, float_value (DoubleEq (0.01))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1E-2").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, OneExpMinusZero2) {
    EXPECT_CALL (callbacks_, float_value (DoubleEq (0.01))).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1E-02").eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, IntegerMax) {
    auto const long_max = std::numeric_limits<long>::max ();
    auto const str_max = std::to_string (long_max);

    EXPECT_CALL (callbacks_, integer_value (long_max)).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input (str_max).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, IntegerMin) {
    auto const long_min = std::numeric_limits<long>::min ();
    auto const str_min = std::to_string (long_min);

    EXPECT_CALL (callbacks_, integer_value (long_min)).Times (1);
    json::parser<decltype (proxy_)> p (proxy_);
    p.input (str_min).eof ();
    EXPECT_FALSE (p.has_error ());
}

TEST_F (JsonNumber, IntegerPositiveOverflow) {
    auto const str =
        std::to_string (static_cast<unsigned long> (std::numeric_limits<long>::max ()) + 1L);

    json::parser<decltype (proxy_)> p (proxy_);
    p.input (str).eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, IntegerNegativeOverflow) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("-123123123123123123123123123123").eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
}

// FIXME: is this test testing what it claims to?
TEST_F (JsonNumber, IntegerNegativeOverflow2) {
    constexpr auto min = std::numeric_limits<long>::min ();
    auto const str = std::to_string (static_cast<unsigned long long> (min) + 1L);

    json::parser<decltype (proxy_)> p (proxy_);
    p.input (str).eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, RealPositiveOverflow) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("123123e100000").eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, RealPositiveOverflow2) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("9999E999").eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, RealUnderflow) {
    json::parser<decltype (proxy_)> p = json::make_parser (proxy_);
    p.input ("123e-10000000").eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
}

TEST_F (JsonNumber, BadExponentDigit) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1Ex").eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
}

TEST_F (JsonNumber, BadFractionDigit) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1..").eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
}
TEST_F (JsonNumber, BadExponentAfterPoint) {
    json::parser<decltype (proxy_)> p (proxy_);
    p.input ("1.E").eof ();
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
}
// eof: unittests/pstore_json/test_number.cpp
