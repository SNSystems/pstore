//*                                                              *
//*  ___ _ __   __ _ _ __ ___  ___    __ _ _ __ _ __ __ _ _   _  *
//* / __| '_ \ / _` | '__/ __|/ _ \  / _` | '__| '__/ _` | | | | *
//* \__ \ |_) | (_| | |  \__ \  __/ | (_| | |  | | | (_| | |_| | *
//* |___/ .__/ \__,_|_|  |___/\___|  \__,_|_|  |_|  \__,_|\__, | *
//*     |_|                                               |___/  *
//===- unittests/pstore_mcrepo/test_sparse_array.cpp ----------------------===//
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

#include "pstore_mcrepo/sparse_array.hpp"
#include <array>
#include <set>
#include <vector>
#include "gmock/gmock.h"

using namespace pstore::repo;

TEST (RepoSparseArray, InitializerListIndicesHasIndex) {
    auto arrp = sparse_array<int>::make_unique ({0, 2, 4});
    std::array<bool, 256> indices{{true, false, true, false, true}};

    for (auto ctr = 0U; ctr < indices.size (); ++ctr) {
        EXPECT_EQ (arrp->has_index (ctr), indices[ctr]);
    }
}

TEST (RepoSparseArray, InitializeWithIndexAndValue) {
    auto arrp = sparse_array<int>::make_unique ({0, 2, 4}, {1, 2, 3});
    auto & arr = *arrp;

    EXPECT_EQ (arr.size (), 3U);
    EXPECT_TRUE (arr.has_index (0));
    EXPECT_FALSE (arr.has_index (1));
    EXPECT_TRUE (arr.has_index (2));
    EXPECT_FALSE (arr.has_index (3));
    EXPECT_FALSE (arr.has_index (256));
    EXPECT_EQ (arr[0], 1);
    EXPECT_EQ (arr[2], 2);
    EXPECT_EQ (arr[4], 3);
}

TEST (RepoSparseArray, Assign) {
    auto arrp = sparse_array<int>::make_unique ({0, 2, 4});
    auto & arr = *arrp;

    arr[0] = 3;
    arr[2] = 5;
    arr[4] = 7;

    EXPECT_EQ (arr[0], 3);
    EXPECT_EQ (arr[2], 5);
    EXPECT_EQ (arr[4], 7);
    arr[4] = 11;
    EXPECT_EQ (arr[0], 3);
    EXPECT_EQ (arr[2], 5);
    EXPECT_EQ (arr[4], 11);
}

TEST (RepoSparseArray, IndexInitializationList) {
    std::string empty;

    auto arr = sparse_array<std::string>::make_unique ({0, 2, 4});
    for (std::string const & v : *arr) {
        EXPECT_EQ (v, empty);
    }

    EXPECT_EQ (std::get<2> (*arr), empty);
}

namespace {
    struct ctor_counter {
        ctor_counter ()
                : v{ctors++} {}
        unsigned v;
        static unsigned ctors;
    };
    unsigned ctor_counter::ctors;
} // namespace

TEST (RepoSparseArray, IndexInitializationListCtorCheck) {
    ctor_counter::ctors = 0;

    auto arrp = sparse_array<ctor_counter>::make_unique ({0, 2, 4});
    auto & arr = *arrp;
    EXPECT_EQ (arr[0].v, 0U);
    EXPECT_EQ (arr[2].v, 1U);
    EXPECT_EQ (arr[4].v, 2U);
}

TEST (RepoSparseArray, IteratorInitialization) {
    std::array<std::size_t, 3> i1{{0, 2, 4}};
    std::array<int, 3> v1{{1, 2, 3}};

    auto arrp = sparse_array<int>::make_unique (std::begin (i1), std::end (i1), std::begin (v1),
                                                std::end (v1));
    auto & arr = *arrp;

    EXPECT_EQ (arr[0], 1);
    EXPECT_FALSE (arr.has_index (1));
    EXPECT_EQ (arr[2], 2);
    EXPECT_FALSE (arr.has_index (3));
    EXPECT_EQ (arr[4], 3);
}

TEST (RepoSparseArray, IteratorInitializationTooFewValues) {
    std::vector<std::size_t> i1{0, 2, 4};
    std::vector<int> v1{1};

    auto arrp = sparse_array<int>::make_unique (std::begin (i1), std::end (i1), std::begin (v1),
                                                std::end (v1));
    auto & arr = *arrp;

    EXPECT_EQ (arr[0], 1);
    EXPECT_FALSE (arr.has_index (1));
    EXPECT_EQ (arr[2], 0);
    EXPECT_FALSE (arr.has_index (3));
    EXPECT_EQ (arr[4], 0);
}

TEST (RepoSparseArray, IteratorInitializationTooManyValues) {
    std::vector<std::size_t> i1{3, 5};
    std::vector<int> v1{3, 5, 7};

    auto arrp = sparse_array<int>::make_unique (std::begin (i1), std::end (i1), std::begin (v1),
                                                std::end (v1));
    auto & arr = *arrp;

    EXPECT_FALSE (arr.has_index (0));
    EXPECT_FALSE (arr.has_index (1));
    EXPECT_FALSE (arr.has_index (2));
    EXPECT_EQ (arr[3], 3);
    EXPECT_FALSE (arr.has_index (4));
    EXPECT_EQ (arr[5], 5);
}

TEST (RepoSparseArray, PairInitialization) {
    std::vector<std::pair<std::size_t, char const *>> const src{
        {0, "zero"}, {2, "two"}, {4, "four"}};
    auto arrp = sparse_array<std::string>::make_unique (std::begin (src), std::end (src));
    auto & arr = *arrp;

    EXPECT_EQ (arr[0], "zero");
    EXPECT_FALSE (arr.has_index (1));
    EXPECT_EQ (arr[2], "two");
    EXPECT_FALSE (arr.has_index (3));
    EXPECT_EQ (arr[4], "four");
}

TEST (RepoSparseArray, Iterator) {
    auto arr = sparse_array<char const *>::make_unique ({{0, "zero"}, {2, "two"}, {4, "four"}});

    std::vector<std::string> actual;
    std::copy (std::begin (*arr), std::end (*arr), std::back_inserter (actual));

    std::vector<std::string> const expected{"zero", "two", "four"};
    EXPECT_THAT (actual, ::testing::ContainerEq (expected));
}

TEST (RepoSparseArray, ReverseIterator) {
    auto arr = sparse_array<char const *>::make_unique ({{0, "zero"}, {2, "two"}, {4, "four"}});

    std::vector<std::string> actual;
    std::copy (arr->crbegin (), arr->crend (), std::back_inserter (actual));

    std::vector<std::string> const expected{"four", "two", "zero"};
    EXPECT_THAT (actual, ::testing::ContainerEq (expected));
}

TEST (RepoSparseArray, Fill) {
    auto arr = sparse_array<std::string>::make_unique ({{0, "zero"}, {2, "two"}, {4, "four"}});
    arr->fill ("foo");

    std::vector<std::string> actual;
    std::copy (std::begin (*arr), std::end (*arr), std::back_inserter (actual));
    std::vector<std::string> const expected{"foo", "foo", "foo"};
    EXPECT_THAT (actual, ::testing::ContainerEq (expected));
}

TEST (RepoSparseArray, Equal) {
    auto arr1 = sparse_array<int>::make_unique ({{0, 0}, {2, 2}, {4, 4}});
    auto arr2 = sparse_array<int>::make_unique ({{0, 0}, {2, 2}, {4, 4}});
    EXPECT_TRUE (*arr1 == *arr2);
}

TEST (RepoSparseArray, Equal2) {
    auto arr1 = sparse_array<int>::make_unique ({{0, 0}, {2, 2}, {4, 5}});
    auto arr2 = sparse_array<int>::make_unique ({{0, 0}, {2, 2}, {4, 4}});
    EXPECT_FALSE (*arr1 == *arr2);
}

TEST (RepoSparseArray, Equal3) {
    auto arr1 = sparse_array<int>::make_unique ({{0, 1}, {2, 2}, {5, 4}});
    auto arr2 = sparse_array<int>::make_unique ({{0, 0}, {2, 2}, {4, 4}});
    EXPECT_FALSE (*arr1 == *arr2);
}

TEST (RepoSparseArray, HasIndex) {
    std::set<std::size_t> indices{2, 3, 5, 7};
    auto arr = sparse_array<int>::make_unique (std::begin (indices), std::end (indices), {});
    EXPECT_EQ (arr->has_index (0), indices.find (0) != indices.end ());
    EXPECT_EQ (arr->has_index (1), indices.find (1) != indices.end ());
    EXPECT_EQ (arr->has_index (2), indices.find (2) != indices.end ());
    EXPECT_EQ (arr->has_index (3), indices.find (3) != indices.end ());
    EXPECT_EQ (arr->has_index (4), indices.find (4) != indices.end ());
    EXPECT_EQ (arr->has_index (5), indices.find (5) != indices.end ());
    EXPECT_EQ (arr->has_index (6), indices.find (6) != indices.end ());
    EXPECT_EQ (arr->has_index (7), indices.find (7) != indices.end ());
}

TEST (RepoSparseArray, Indices) {
    std::set<std::size_t> indices{2, 3, 5, 7};
    auto arr = sparse_array<int>::make_unique (std::begin (indices), std::end (indices), {});

    sparse_array<int>::indices idc{*arr};
    std::vector<std::size_t> actual;
    std::copy (std::begin (idc), std::end (idc), std::back_inserter (actual));
    std::vector<std::size_t> const expected{2, 3, 5, 7};
    EXPECT_THAT (actual, ::testing::ContainerEq (expected));
}

TEST (RepoSparseArray, SizeBytesAgree) {
    std::vector<std::pair<int, int>> empty;
    EXPECT_EQ (sparse_array<int>::make_unique (std::begin (empty), std::end (empty))->size_bytes (),
               sparse_array<int>::size_bytes (0));

    EXPECT_EQ (sparse_array<int>::make_unique ({0})->size_bytes (),
               sparse_array<int>::size_bytes (1));
    EXPECT_EQ (sparse_array<int>::make_unique ({1, 3})->size_bytes (),
               sparse_array<int>::size_bytes (2));
    EXPECT_EQ (sparse_array<int>::make_unique ({1, 3, 5, 7, 11})->size_bytes (),
               sparse_array<int>::size_bytes (5));
}

TEST (RepoSparseArray, FrontAndBack) {
    std::vector<std::size_t> indices{2, 3, 5, 7};
    auto arr =
        sparse_array<int>::make_unique (std::begin (indices), std::end (indices), {11, 13, 17, 19});
    EXPECT_EQ (arr->front (), 11);
    EXPECT_EQ (arr->back (), 19);
}

// eof: unittests/pstore_mcrepo/test_sparse_array.cpp
