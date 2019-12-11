//*                              *
//*   __ _ _ __ _ __ __ _ _   _  *
//*  / _` | '__| '__/ _` | | | | *
//* | (_| | |  | | | (_| | |_| | *
//*  \__,_|_|  |_|  \__,_|\__, | *
//*                       |___/  *
//===- unittests/dump/test_array.cpp --------------------------------------===//
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
#include "pstore/dump/value.hpp"
#include <memory>
#include "gtest/gtest.h"
#include "convert.hpp"

namespace {

    template <typename CharType>
    class Array : public ::testing::Test {
    public:
        void SetUp () final {}
        void TearDown () final {}

    protected:
        std::basic_ostringstream<CharType> out;
    };

} // end anonymous namespace

using CharacterTypes = ::testing::Types<char, wchar_t>;
TYPED_TEST_SUITE (Array, CharacterTypes, );


TYPED_TEST (Array, Empty) {
    pstore::dump::array arr;
    arr.write (this->out);
    auto const actual = this->out.str ();
    auto const & expected = convert<TypeParam> ("[ ]");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Array, TwoNumbers) {
    using namespace ::pstore::dump;
    array arr ({make_number (3), make_number (5)});
    arr.write (this->out);
    auto const & actual = this->out.str ();
    auto const & expected = convert<TypeParam> ("[ 0x3, 0x5 ]");
    EXPECT_EQ (expected, actual);
}

TYPED_TEST (Array, TwoStrings) {
    using namespace ::pstore::dump;
    array arr ({
        make_value ("Hello"),
        make_value ("World"),
    });
    arr.write (this->out);
    auto const & actual = this->out.str ();
    auto const & expected = convert<TypeParam> ("\n"
                                                "- Hello\n"
                                                "- World");

    EXPECT_EQ (expected, actual);
}
