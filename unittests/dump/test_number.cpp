//*                        _                *
//*  _ __  _   _ _ __ ___ | |__   ___ _ __  *
//* | '_ \| | | | '_ ` _ \| '_ \ / _ \ '__| *
//* | | | | |_| | | | | | | |_) |  __/ |    *
//* |_| |_|\__,_|_| |_| |_|_.__/ \___|_|    *
//*                                         *
//===- unittests/dump/test_number.cpp -------------------------------------===//
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
#include "pstore/dump/value.hpp"
// System includes
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <sstream>
// 3rd party includes
#include "gtest/gtest.h"
// Local includes
#include "convert.hpp"

namespace {
    template <typename CharType>
    class Number : public testing::Test {
    public:
        Number () : base_ {pstore::dump::number_base::default_base ()} {}
        ~Number () {
            using namespace ::pstore::dump;
            switch (base_) {
            case 8: number_base::oct (); break;
            case 10: number_base::dec (); break;
            case 16: number_base::hex (); break;
            default: assert (false); break;
            }
        }

    protected:
        std::basic_ostringstream<CharType> out_;

    private:
        unsigned const base_ = 0;
    };
} // namespace

typedef ::testing::Types<char, wchar_t> CharacterTypes;
TYPED_TEST_CASE (Number, CharacterTypes);

TYPED_TEST (Number, N0ExplicitBase8) {
    pstore::dump::number_long v (0, 8);
    v.write (this->out_);
    auto const & actual = this->out_.str ();
    auto const & expected = convert<TypeParam> ("0");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N0ExplicitBase10) {
    pstore::dump::number_long v (0, 10);
    v.write (this->out_);
    auto const & actual = this->out_.str ();
    auto const & expected = convert<TypeParam> ("0");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N0ExplicitBase16) {
    pstore::dump::number_long v (0, 16);
    v.write (this->out_);
    auto const & actual = this->out_.str ();
    auto const & expected = convert<TypeParam> ("0x0");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N10ExplicitBase10) {
    pstore::dump::number_long v (10, 10);
    v.write (this->out_);
    auto const & actual = this->out_.str ();
    auto const & expected = convert<TypeParam> ("10");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N15ExplicitBase16) {
    pstore::dump::number_long v (15, 16);
    v.write (this->out_);
    auto const & actual = this->out_.str ();
    auto const & expected = convert<TypeParam> ("0xf");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N10DefaultBase8) {
    pstore::dump::number_base::oct ();
    pstore::dump::number_long v (10);
    v.write (this->out_);
    auto const & actual = this->out_.str ();
    auto const & expected = convert<TypeParam> ("012");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N10DefaultBase10) {
    pstore::dump::number_base::dec ();
    pstore::dump::number_long v (10, 10);
    v.write (this->out_);
    auto const & actual = this->out_.str ();
    auto const & expected = convert<TypeParam> ("10");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Number, N10DefaultBase16) {
    pstore::dump::number_base::hex ();
    pstore::dump::number_long v (10, 16);
    v.write (this->out_);
    auto const & actual = this->out_.str ();
    auto const & expected = convert<TypeParam> ("0xa");
    EXPECT_EQ (expected, actual);
}
