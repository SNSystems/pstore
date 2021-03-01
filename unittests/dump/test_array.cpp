//===- unittests/dump/test_array.cpp --------------------------------------===//
//*                              *
//*   __ _ _ __ _ __ __ _ _   _  *
//*  / _` | '__| '__/ _` | | | | *
//* | (_| | |  | | | (_| | |_| | *
//*  \__,_|_|  |_|  \__,_|\__, | *
//*                       |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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

    using CharacterTypes = ::testing::Types<char, wchar_t>;

} // end anonymous namespace

#ifdef PSTORE_IS_INSIDE_LLVM
TYPED_TEST_CASE (Array, CharacterTypes);
#else
TYPED_TEST_SUITE (Array, CharacterTypes, );
#endif


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
