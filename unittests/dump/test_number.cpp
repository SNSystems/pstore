//===- unittests/dump/test_number.cpp -------------------------------------===//
//*                        _                *
//*  _ __  _   _ _ __ ___ | |__   ___ _ __  *
//* | '_ \| | | | '_ ` _ \| '_ \ / _ \ '__| *
//* | | | | |_| | | | | | | |_) |  __/ |    *
//* |_| |_|\__,_|_| |_| |_|_.__/ \___|_|    *
//*                                         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/dump/value.hpp"

// System includes
#include <cstdlib>
#include <cstring>
#include <sstream>

// 3rd party includes
#include <gtest/gtest.h>

// Local includes
#include "convert.hpp"

namespace {

    template <typename CharType>
    class Number : public testing::Test {
    public:
        Number ()
                : base_{pstore::dump::number_base::default_base ()} {}
        ~Number () override {
            using namespace ::pstore::dump;
            switch (base_) {
            case 8: number_base::oct (); break;
            case 10: number_base::dec (); break;
            case 16: number_base::hex (); break;
            default: PSTORE_ASSERT (false); break;
            }
        }

    protected:
        std::basic_ostringstream<CharType> out_;

    private:
        unsigned const base_ = 0;
    };

    using CharacterTypes = ::testing::Types<char, wchar_t>;

} // end anonymous namespace

#ifdef PSTORE_IS_INSIDE_LLVM
TYPED_TEST_CASE (Number, CharacterTypes);
#else
TYPED_TEST_SUITE (Number, CharacterTypes, );
#endif // PSTORE_IS_INSIDE_LLVM

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

TYPED_TEST (Number, NegativeExplicitBase10) {
    const auto value = std::numeric_limits<long long>::min ();
    pstore::dump::number_long v (value, 10);
    v.write (this->out_);
    auto const & expected = convert<TypeParam> (std::to_string (value).c_str ());
    EXPECT_EQ (expected, this->out_.str ());
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
