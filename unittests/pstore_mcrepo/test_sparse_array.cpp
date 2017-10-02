//*                                                              *
//*  ___ _ __   __ _ _ __ ___  ___    __ _ _ __ _ __ __ _ _   _  *
//* / __| '_ \ / _` | '__/ __|/ _ \  / _` | '__| '__/ _` | | | | *
//* \__ \ |_) | (_| | |  \__ \  __/ | (_| | |  | | | (_| | |_| | *
//* |___/ .__/ \__,_|_|  |___/\___|  \__,_|_|  |_|  \__,_|\__, | *
//*     |_|                                               |___/  *
//===- unittests/pstore_mcrepo/test_sparse_array.cpp ----------------------===//
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

#include "pstore_mcrepo/sparse_array.hpp"
#include <array>
#include <set>
#include <vector>
#include "gmock/gmock.h"

using namespace pstore::repo;

TEST(RepoSparseArray, InitializerListIndicesHasIndex) {
  auto Arrp = SparseArray<int>::make_unique({0, 2, 4});
  std::array<bool, 256> Indices{{true, false, true, false, true}};

  for (auto Ctr = 0U; Ctr < Indices.size(); ++Ctr) {
    EXPECT_EQ(Arrp->has_index(Ctr), Indices[Ctr]);
  }
}

TEST(RepoSparseArray, InitializeWithIndexAndValue) {
  auto Arrp = SparseArray<int>::make_unique({0, 2, 4}, {1, 2, 3});
  auto &Arr = *Arrp;

  EXPECT_EQ(Arr.size(), 3U);
  EXPECT_TRUE(Arr.has_index(0));
  EXPECT_FALSE(Arr.has_index(1));
  EXPECT_TRUE(Arr.has_index(2));
  EXPECT_FALSE(Arr.has_index(3));
  EXPECT_FALSE(Arr.has_index(256));
  EXPECT_EQ(Arr[0], 1);
  EXPECT_EQ(Arr[2], 2);
  EXPECT_EQ(Arr[4], 3);
}

TEST(RepoSparseArray, Assign) {
  auto Arrp = SparseArray<int>::make_unique({0, 2, 4});
  auto &Arr = *Arrp;

  Arr[0] = 3;
  Arr[2] = 5;
  Arr[4] = 7;

  EXPECT_EQ(Arr[0], 3);
  EXPECT_EQ(Arr[2], 5);
  EXPECT_EQ(Arr[4], 7);
  Arr[4] = 11;
  EXPECT_EQ(Arr[0], 3);
  EXPECT_EQ(Arr[2], 5);
  EXPECT_EQ(Arr[4], 11);
}

TEST(RepoSparseArray, IndexInitializationList) {
  std::string Empty;

  auto Arr = SparseArray<std::string>::make_unique({0, 2, 4});
  for (std::string const &V : *Arr) {
    EXPECT_EQ(V, Empty);
  }

  EXPECT_EQ(std::get<2>(*Arr), Empty);
}

namespace {
    struct CtorCounter {
        CtorCounter () : V {Ctors++} {}
        unsigned V;
        static unsigned Ctors;
    };
    unsigned CtorCounter::Ctors;
}

TEST(RepoSparseArray, IndexInitializationListCtorCheck) {
  CtorCounter::Ctors = 0;

  auto Arrp = SparseArray<CtorCounter>::make_unique({0, 2, 4});
  auto & Arr = *Arrp;
  EXPECT_EQ (Arr [0].V, 0U);
  EXPECT_EQ (Arr [2].V, 1U);
  EXPECT_EQ (Arr [4].V, 2U);
}

TEST(RepoSparseArray, IteratorInitialization) {
  std::array<std::size_t, 3> I1{{0, 2, 4}};
  std::array<int, 3> V1{{1, 2, 3}};

  auto Arrp = SparseArray<int>::make_unique(std::begin(I1), std::end(I1),
                                        std::begin(V1), std::end(V1));
  auto &Arr = *Arrp;

  EXPECT_EQ(Arr[0], 1);
  EXPECT_FALSE(Arr.has_index(1));
  EXPECT_EQ(Arr[2], 2);
  EXPECT_FALSE(Arr.has_index(3));
  EXPECT_EQ(Arr[4], 3);
}

TEST(RepoSparseArray, IteratorInitializationTooFewValues) {
  std::vector<std::size_t> I1{0, 2, 4};
  std::vector<int> V1{1};

  auto Arrp = SparseArray<int>::make_unique(std::begin(I1), std::end(I1),
                                        std::begin(V1), std::end(V1));
  auto &Arr = *Arrp;

  EXPECT_EQ(Arr[0], 1);
  EXPECT_FALSE(Arr.has_index(1));
  EXPECT_EQ(Arr[2], 0);
  EXPECT_FALSE(Arr.has_index(3));
  EXPECT_EQ(Arr[4], 0);
}

TEST(RepoSparseArray, IteratorInitializationTooManyValues) {
  std::vector<std::size_t> I1{3, 5};
  std::vector<int> V1{3, 5, 7};

  auto Arrp = SparseArray<int>::make_unique(std::begin(I1), std::end(I1),
                                        std::begin(V1), std::end(V1));
  auto &Arr = *Arrp;

  EXPECT_FALSE(Arr.has_index(0));
  EXPECT_FALSE(Arr.has_index(1));
  EXPECT_FALSE(Arr.has_index(2));
  EXPECT_EQ(Arr[3], 3);
  EXPECT_FALSE(Arr.has_index(4));
  EXPECT_EQ(Arr[5], 5);
}

TEST(RepoSparseArray, PairInitialization) {
  std::vector<std::pair<std::size_t, char const *>> const Src{
      {0, "zero"}, {2, "two"}, {4, "four"}};
  auto Arrp = SparseArray<std::string>::make_unique(std::begin(Src), std::end(Src));
  auto &Arr = *Arrp;

  EXPECT_EQ(Arr[0], "zero");
  EXPECT_FALSE(Arr.has_index(1));
  EXPECT_EQ(Arr[2], "two");
  EXPECT_FALSE(Arr.has_index(3));
  EXPECT_EQ(Arr[4], "four");
}

TEST(RepoSparseArray, Iterator) {
  auto Arr = SparseArray<char const *>::make_unique(
      {{0, "zero"}, {2, "two"}, {4, "four"}});

  std::vector<std::string> actual;
  std::copy(std::begin(*Arr), std::end(*Arr), std::back_inserter(actual));

  std::vector<std::string> const expected{"zero", "two", "four"};
  EXPECT_THAT(actual, ::testing::ContainerEq(expected));
}

TEST(RepoSparseArray, ReverseIterator) {
  auto Arr = SparseArray<char const *>::make_unique(
      {{0, "zero"}, {2, "two"}, {4, "four"}});

  std::vector<std::string> Actual;
  std::copy(Arr->crbegin(), Arr->crend(), std::back_inserter(Actual));

  std::vector<std::string> const expected{"four", "two", "zero"};
  EXPECT_THAT(Actual, ::testing::ContainerEq(expected));
}

TEST(RepoSparseArray, Fill) {
  auto Arr =
      SparseArray<std::string>::make_unique({{0, "zero"}, {2, "two"}, {4, "four"}});
  Arr->fill("foo");

  std::vector<std::string> Actual;
  std::copy(std::begin(*Arr), std::end(*Arr), std::back_inserter(Actual));
  std::vector<std::string> const expected{"foo", "foo", "foo"};
  EXPECT_THAT(Actual, ::testing::ContainerEq(expected));
}

TEST(RepoSparseArray, Equal) {
  auto Arr1 = SparseArray<int>::make_unique({{0, 0}, {2, 2}, {4, 4}});
  auto Arr2 = SparseArray<int>::make_unique({{0, 0}, {2, 2}, {4, 4}});
  EXPECT_TRUE(*Arr1 == *Arr2);
}

TEST(RepoSparseArray, Equal2) {
  auto Arr1 = SparseArray<int>::make_unique({{0, 0}, {2, 2}, {4, 5}});
  auto Arr2 = SparseArray<int>::make_unique({{0, 0}, {2, 2}, {4, 4}});
  EXPECT_FALSE(*Arr1 == *Arr2);
}

TEST(RepoSparseArray, Equal3) {
  auto Arr1 = SparseArray<int>::make_unique({{0, 1}, {2, 2}, {5, 4}});
  auto Arr2 = SparseArray<int>::make_unique({{0, 0}, {2, 2}, {4, 4}});
  EXPECT_FALSE(*Arr1 == *Arr2);
}

TEST(RepoSparseArray, HasIndex) {
  std::set<std::size_t> Indices{2, 3, 5, 7};
  auto Arr =
      SparseArray<int>::make_unique(std::begin(Indices), std::end(Indices), {});
  EXPECT_EQ(Arr->has_index(0), Indices.find(0) != Indices.end());
  EXPECT_EQ(Arr->has_index(1), Indices.find(1) != Indices.end());
  EXPECT_EQ(Arr->has_index(2), Indices.find(2) != Indices.end());
  EXPECT_EQ(Arr->has_index(3), Indices.find(3) != Indices.end());
  EXPECT_EQ(Arr->has_index(4), Indices.find(4) != Indices.end());
  EXPECT_EQ(Arr->has_index(5), Indices.find(5) != Indices.end());
  EXPECT_EQ(Arr->has_index(6), Indices.find(6) != Indices.end());
  EXPECT_EQ(Arr->has_index(7), Indices.find(7) != Indices.end());
}

TEST(RepoSparseArray, Indices) {
  std::set<std::size_t> Indices{2, 3, 5, 7};
  auto Arr =
      SparseArray<int>::make_unique(std::begin(Indices), std::end(Indices), {});

  SparseArray<int>::Indices Idc{*Arr};
  std::vector<std::size_t> Actual;
  std::copy(std::begin(Idc), std::end(Idc), std::back_inserter(Actual));
  std::vector<std::size_t> const Expected{2, 3, 5, 7};
  EXPECT_THAT(Actual, ::testing::ContainerEq(Expected));
}

TEST(RepoSparseArray, SizeBytesAgree) {
  std::vector<std::pair<int, int>> Empty;
  EXPECT_EQ(SparseArray<int>::make_unique(std::begin(Empty), std::end(Empty))
                ->size_bytes(),
            SparseArray<int>::size_bytes(0));

  EXPECT_EQ(SparseArray<int>::make_unique({0})->size_bytes(),
            SparseArray<int>::size_bytes(1));
  EXPECT_EQ(SparseArray<int>::make_unique({1, 3})->size_bytes(),
            SparseArray<int>::size_bytes(2));
  EXPECT_EQ(SparseArray<int>::make_unique({1, 3, 5, 7, 11})->size_bytes(),
            SparseArray<int>::size_bytes(5));
}
// eof: unittests/pstore_mcrepo/test_sparse_array.cpp
