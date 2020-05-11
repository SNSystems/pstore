//*       _                 _            _                  _              *
//*   ___| |__  _   _ _ __ | | _____  __| | __   _____  ___| |_ ___  _ __  *
//*  / __| '_ \| | | | '_ \| |/ / _ \/ _` | \ \ / / _ \/ __| __/ _ \| '__| *
//* | (__| | | | |_| | | | |   <  __/ (_| |  \ V /  __/ (__| || (_) | |    *
//*  \___|_| |_|\__,_|_| |_|_|\_\___|\__,_|   \_/ \___|\___|\__\___/|_|    *
//*                                                                        *
//===- unittests/adt/test_chunked_vector.cpp ------------------------------===//
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
#include "pstore/adt/chunked_vector.hpp"

#include <utility>

#include <gmock/gmock.h>

TEST (ChunkedVector, Init) {
    pstore::chunked_vector<int> cv;
    EXPECT_EQ (cv.size (), 0U);
    EXPECT_EQ (std::distance (std::begin (cv), std::end (cv)), 0);
}

TEST (ChunkedVector, OneMember) {
    pstore::chunked_vector<int> cv;
    cv.emplace_back (1);

    EXPECT_EQ (cv.size (), 1U);
    auto it = cv.begin ();
    EXPECT_EQ (*it, 1);
    ++it;
    EXPECT_EQ (it, cv.end ());
}

TEST (ChunkedVector, Swap) {
    pstore::chunked_vector<int> a, b;
    a.emplace_back (7);
    std::swap (a, b);
    EXPECT_EQ (a.size (), 0U);
    ASSERT_EQ (b.size (), 1U);
    EXPECT_EQ (b.front (), 7);
}

TEST (ChunkedVector, Splice) {
    pstore::chunked_vector<int> a;
    a.emplace_back (7);

    pstore::chunked_vector<int> b;
    b.emplace_back (11);

    a.splice (std::move (b));
    EXPECT_THAT (a, testing::ElementsAre (7, 11));
}

TEST (ChunkedVector, IteratorAssign) {
    pstore::chunked_vector<int> cv;
    cv.emplace_back (7);
    pstore::chunked_vector<int>::iterator it = cv.begin ();
    pstore::chunked_vector<int>::iterator it2 = it;         // non-const copy ctor from non-const
    it2 = it;                                               // non-const assign from non-const.
    pstore::chunked_vector<int>::const_iterator cit = it;   // const copy ctor from non-const.
    pstore::chunked_vector<int>::const_iterator cit2 = cit; // copy ctor from const.
    cit2 = cit;                                             // assign from const.
    cit2 = it;                                              // assign from non-const.
    pstore::chunked_vector<int>::const_iterator cit3 = std::move (cit); // move ctor from const.
    EXPECT_EQ (*cit3, 7);
}

namespace {

    // Limit the chunks to two elements each.
    using vector_type = pstore::chunked_vector<int, std::size_t{2}>;

    template <typename IteratorType>
    class CVIterator : public testing::Test {
    public:
        CVIterator () {
            cv_.reserve (4);
            cv_.emplace_back (2);
            cv_.emplace_back (3);
            cv_.emplace_back (5);
            cv_.emplace_back (7);
        }

    protected:
        vector_type cv_;
    };

} // end anonymous namespace

using IteratorTypes = testing::Types<vector_type::iterator, vector_type::const_iterator>;
TYPED_TEST_SUITE (CVIterator, IteratorTypes, );

TYPED_TEST (CVIterator, Preincrement) {
    EXPECT_EQ (this->cv_.size (), 4U);

    TypeParam it = this->cv_.begin ();
    EXPECT_EQ (*it, 2);
    ++it;
    EXPECT_EQ (*it, 3);
    ++it;
    EXPECT_EQ (*it, 5);
    ++it;
    EXPECT_EQ (*it, 7);
    ++it;
    EXPECT_EQ (it, this->cv_.end ());
}

TYPED_TEST (CVIterator, Predecrement) {
    EXPECT_EQ (this->cv_.size (), 4U);

    TypeParam it = this->cv_.end ();
    --it;
    EXPECT_EQ (*it, 7);
    --it;
    EXPECT_EQ (*it, 5);
    --it;
    EXPECT_EQ (*it, 3);
    --it;
    EXPECT_EQ (*it, 2);
    EXPECT_EQ (it, this->cv_.begin ());
}
