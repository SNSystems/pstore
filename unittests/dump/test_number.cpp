//*                        _                *
//*  _ __  _   _ _ __ ___ | |__   ___ _ __  *
//* | '_ \| | | | '_ ` _ \| '_ \ / _ \ '__| *
//* | | | | |_| | | | | | | |_) |  __/ |    *
//* |_| |_|\__,_|_| |_| |_|_.__/ \___|_|    *
//*                                         *
//===- unittests/dump/test_number.cpp -------------------------------------===//
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
#include "dump/value.hpp"
// System includes
#include <cstdlib>
#include <cstring>
#include <sstream>
// 3rd partly includes
#include "gtest/gtest.h"
// Local includes
#include "convert.hpp"

namespace {
    template <typename CharType>
        class Number : public ::testing::Test {
        public:
            void SetUp () final {
                base_ = value::number_base::default_base ();
            }
            void TearDown () final {
                switch (base_) {
                case 8: value::number_base::oct (); break;
                case 10: value::number_base::dec (); break;
                case 16: value::number_base::hex (); break;
                default: ASSERT_TRUE (false); break;
                }
            }

        protected:
            std::basic_ostringstream <CharType> out;

        private:
            unsigned base_ = 0;
        };
}

typedef ::testing::Types <char, wchar_t>  CharacterTypes;
TYPED_TEST_CASE (Number, CharacterTypes);

TYPED_TEST (Number, N0ExplicitBase8) {
    value::number <int> v (0, 8);
    v.write (this->out);
    auto const & actual = this->out.str ();
    auto const & expected = convert <TypeParam> ("0");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N0ExplicitBase10) {
    value::number <int> v (0, 10);
    v.write (this->out);
    auto const & actual = this->out.str ();
    auto const & expected = convert <TypeParam> ("0");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N0ExplicitBase16) {
    value::number <int> v (0, 16);
    v.write (this->out);
    auto const & actual = this->out.str ();
    auto const & expected = convert <TypeParam> ("0x0");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N10ExplicitBase10) {
    value::number <int> v (10, 10);
    v.write (this->out);
    auto const & actual = this->out.str ();
    auto const & expected = convert <TypeParam> ("10");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N15ExplicitBase16) {
    value::number <int> v (15, 16);
    v.write (this->out);
    auto const & actual = this->out.str ();
    auto const & expected = convert <TypeParam> ("0xf");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N10DefaultBase8) {
    value::number_base::oct ();
    value::number <int> v (10);
    v.write (this->out);
    auto const & actual = this->out.str ();
    auto const & expected = convert <TypeParam> ("012");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N10DefaultBase10) {
    value::number_base::dec ();
    value::number <int> v (10, 10);
    v.write (this->out);
    auto const & actual = this->out.str ();
    auto const & expected = convert <TypeParam> ("10");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N10DefaultBase16) {
    value::number_base::hex ();
    value::number <int> v (10, 16);
    v.write (this->out);
    auto const & actual = this->out.str ();
    auto const & expected = convert <TypeParam> ("0xa");
    EXPECT_EQ (expected, actual);
}
// eof: unittests/dump/test_number.cpp
