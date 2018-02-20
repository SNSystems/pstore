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
#include "pstore_json/json.hpp"
#include "pstore_json/dom_types.hpp"
#include "gtest/gtest.h"

using namespace pstore;

TEST (Number, Zero) {
    json::parser<json::yaml_output> p;
    p.parse ("0");
    std::shared_ptr<json::value::dom_element> v = p.eof ();

    ASSERT_NE (v, nullptr);
    ASSERT_NE (v->as_long (), nullptr);
    EXPECT_EQ (v->as_long ()->get (), 0L);
}

TEST (Number, MinusOne) {
    json::parser<json::yaml_output> p;
    p.parse ("-1");
    std::shared_ptr<json::value::dom_element> v = p.eof ();

    ASSERT_NE (v, nullptr);
    ASSERT_NE (v->as_long (), nullptr);
    EXPECT_EQ (v->as_long ()->get (), -1L);
}

TEST (Number, MinusMinus) {
    json::parser<json::yaml_output> p;
    p.parse ("--");
    std::shared_ptr<json::value::dom_element> v = p.eof ();

    ASSERT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
}

TEST (Number, OneTwoThree) {
    json::parser<json::yaml_output> p;
    p.parse ("123");
    std::shared_ptr<json::value::dom_element> v = p.eof ();

    ASSERT_NE (v, nullptr);
    ASSERT_NE (v->as_long (), nullptr);
    EXPECT_EQ (v->as_long ()->get (), 123L);
}

TEST (Number, Pi) {
    {
        json::parser<json::yaml_output> p;
        p.parse ("3.1415");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 3.1415);
    }
    {
        json::parser<json::yaml_output> p;
        p.parse ("-3.1415");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), -3.1415);
    }
}

TEST (Number, Point45) {
    {
        json::parser<json::yaml_output> p;
        p.parse ("0.45");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 0.45);
    }
    {
        json::parser<json::yaml_output> p;
        p.parse ("-0.45");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), -0.45);
    }
}

TEST (Number, ZeroExp2) {
    json::parser<json::yaml_output> p;
    p.parse ("0e2");
    std::shared_ptr<json::value::dom_element> v = p.eof ();

    ASSERT_NE (v, nullptr);
    ASSERT_NE (v->as_double (), nullptr);
    ASSERT_DOUBLE_EQ (v->as_double ()->get (), 0.0);
}

TEST (Number, OneExp2) {
    {
        json::parser<json::yaml_output> p;
        p.parse ("1e2");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 100.0);
    }
    {
        json::parser<json::yaml_output> p;
        p.parse ("1e+2");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 100.0);
    }
}

TEST (Number, OneExpMinus2) {
    {
        json::parser<json::yaml_output> p;
        p.parse ("0.01");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 0.01);
    }
    {
        json::parser<json::yaml_output> p;
        p.parse ("1e-2");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 0.01);
    }
    {
        json::parser<json::yaml_output> p;
        p.parse ("1E-2");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 0.01);
    }
    {
        json::parser<json::yaml_output> p;
        p.parse ("1E-02");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_double (), nullptr);
        ASSERT_DOUBLE_EQ (v->as_double ()->get (), 0.01);
    }
}

TEST (Number, IntegerMaxAndMin) {
    {
        auto const long_max = std::numeric_limits<long>::max ();
        auto const str_max = std::to_string (long_max);

        json::parser<json::yaml_output> p;
        p.parse (str_max);
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_long (), nullptr);
        EXPECT_EQ (v->as_long ()->get (), long_max);
    }
    {
        auto const long_min = std::numeric_limits<long>::min ();
        auto const str_min = std::to_string (long_min);

        json::parser<json::yaml_output> p;
        p.parse (str_min);
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        ASSERT_NE (v, nullptr);
        ASSERT_NE (v->as_long (), nullptr);
        EXPECT_EQ (v->as_long ()->get (), long_min);
    }
}

TEST (Number, IntegerPositiveOverflow) {
    auto const str =
        std::to_string (static_cast<unsigned long> (std::numeric_limits<long>::max ()) + 1L);

    json::parser<json::yaml_output> p;
    p.parse (str);
    std::shared_ptr<json::value::dom_element> v = p.eof ();

    EXPECT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
}

TEST (Number, IntegerNegativeOverflow) {
    {
        json::parser<json::yaml_output> p;
        p.parse ("-123123123123123123123123123123");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
    }
    {
        constexpr auto min = std::numeric_limits<long>::min ();
        auto const str = std::to_string (static_cast<unsigned long long> (min) + 1L);

        json::parser<json::yaml_output> p;
        p.parse (str);
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
    }
}

TEST (Number, RealPositiveOverflow) {
    {
        json::parser<json::yaml_output> p;
        p.parse ("123123e100000");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
    }
    {
        json::parser<json::yaml_output> p;
        p.parse ("9999E999");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::number_out_of_range));
    }
}

TEST (Number, BadExponentDigit) {
    json::parser<json::yaml_output> p;
    p.parse ("1Ex");
    std::shared_ptr<json::value::dom_element> v = p.eof ();

    EXPECT_EQ (v, nullptr);
    EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
}

TEST (Number, BadFractionDigit) {
    {
        json::parser<json::yaml_output> p;
        p.parse ("1..");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
    }
    {
        json::parser<json::yaml_output> p;
        p.parse ("1.E");
        std::shared_ptr<json::value::dom_element> v = p.eof ();

        EXPECT_EQ (v, nullptr);
        EXPECT_EQ (p.last_error (), std::make_error_code (json::error_code::unrecognized_token));
    }
}
// eof: unittests/pstore_json/test_number.cpp
