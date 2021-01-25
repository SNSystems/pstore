//*              _       _                        _             _              *
//*  _ __   ___ (_)_ __ | |_ ___  ___    __ _  __| | __ _ _ __ | |_ ___  _ __  *
//* | '_ \ / _ \| | '_ \| __/ _ \/ _ \  / _` |/ _` |/ _` | '_ \| __/ _ \| '__| *
//* | |_) | (_) | | | | | ||  __/  __/ | (_| | (_| | (_| | |_) | || (_) | |    *
//* | .__/ \___/|_|_| |_|\__\___|\___|  \__,_|\__,_|\__,_| .__/ \__\___/|_|    *
//* |_|                                                  |_|                   *
//===- unittests/support/test_pointee_adaptor.cpp -------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/support/pointee_adaptor.hpp"
#include <algorithm>
#include <array>
#include <vector>
#include "gmock/gmock.h"

namespace {

    class PointeeAdaptor : public ::testing::Test {
    public:
        PointeeAdaptor ()
                : begin_{pstore::make_pointee_adaptor (std::begin (pvalues))}
                , end_{pstore::make_pointee_adaptor (std::end (pvalues))} {}

    protected:
        std::array<int, 2> values{{1, 2}};
        std::array<int *, 2> pvalues{{&values[0], &values[1]}};
        pstore::pointee_adaptor<decltype (pvalues)::iterator> const begin_;
        pstore::pointee_adaptor<decltype (pvalues)::iterator> const end_;
    };

} // namespace

TEST_F (PointeeAdaptor, PreIncrement) {
    auto it = begin_;
    EXPECT_EQ (*it, 1);
    EXPECT_EQ (*++it, 2);
    EXPECT_EQ (++it, end_);
}

TEST_F (PointeeAdaptor, PreDecrement) {
    auto it = end_;
    EXPECT_EQ (*--it, 2);
    EXPECT_EQ (*--it, 1);
}

TEST_F (PointeeAdaptor, PostIncrement) {
    auto it = begin_;
    EXPECT_EQ (*it++, 1);
    EXPECT_EQ (*it++, 2);
    EXPECT_EQ (it, end_);
}

TEST_F (PointeeAdaptor, PostDecrement) {
    auto it = end_ - 1;
    EXPECT_EQ (*it--, 2);
    EXPECT_EQ (*it, 1);
    EXPECT_EQ (it, begin_);
}

TEST_F (PointeeAdaptor, AddN) {
    auto it = begin_;
    EXPECT_EQ (it + 2, end_);
    EXPECT_EQ (2 + it, end_);
    it += 2;
    EXPECT_EQ (it, end_);
}

TEST_F (PointeeAdaptor, SubN) {
    auto it = end_;
    EXPECT_EQ (it - 2, begin_);
    it -= 2;
    EXPECT_EQ (it, begin_);
}

TEST_F (PointeeAdaptor, Index) {
    EXPECT_EQ (begin_[0], 1);
    EXPECT_EQ (begin_[1], 2);
}

TEST_F (PointeeAdaptor, Relational) {
    auto a = begin_;
    auto b = begin_ + 1;
    EXPECT_TRUE (b > a);
    EXPECT_FALSE (a > b);
    EXPECT_TRUE (b >= a);
    EXPECT_FALSE (a > b);
    EXPECT_TRUE (a <= b);
    EXPECT_FALSE (b <= a);
    EXPECT_TRUE (a <= a);
    EXPECT_TRUE (a >= a);
}

TEST_F (PointeeAdaptor, UniquePtr) {
    // Test usage with a container of smart (rather than raw) pointers.
    std::vector<std::unique_ptr<int>> v;
    v.emplace_back (new int (1));
    v.emplace_back (new int (2));

    std::vector<int> out;
    std::copy (pstore::make_pointee_adaptor (std::begin (v)),
               pstore::make_pointee_adaptor (std::end (v)), std::back_inserter (out));
    EXPECT_THAT (out, ::testing::ElementsAre (1, 2));
}
