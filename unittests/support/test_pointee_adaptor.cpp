//*              _       _                        _             _              *
//*  _ __   ___ (_)_ __ | |_ ___  ___    __ _  __| | __ _ _ __ | |_ ___  _ __  *
//* | '_ \ / _ \| | '_ \| __/ _ \/ _ \  / _` |/ _` |/ _` | '_ \| __/ _ \| '__| *
//* | |_) | (_) | | | | | ||  __/  __/ | (_| | (_| | (_| | |_) | || (_) | |    *
//* | .__/ \___/|_|_| |_|\__\___|\___|  \__,_|\__,_|\__,_| .__/ \__\___/|_|    *
//* |_|                                                  |_|                   *
//===- unittests/support/test_pointee_adaptor.cpp -------------------------===//
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
    v.emplace_back (new int(1));
    v.emplace_back (new int(2));

    std::vector<int> out;
    std::copy (pstore::make_pointee_adaptor (std::begin (v)),
               pstore::make_pointee_adaptor (std::end (v)), std::back_inserter (out));
    EXPECT_THAT (out, ::testing::ElementsAre (1, 2));
}
