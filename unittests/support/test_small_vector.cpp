//*                      _ _                  _              *
//*  ___ _ __ ___   __ _| | | __   _____  ___| |_ ___  _ __  *
//* / __| '_ ` _ \ / _` | | | \ \ / / _ \/ __| __/ _ \| '__| *
//* \__ \ | | | | | (_| | | |  \ V /  __/ (__| || (_) | |    *
//* |___/_| |_| |_|\__,_|_|_|   \_/ \___|\___|\__\___/|_|    *
//*                                                          *
//===- unittests/support/test_small_vector.cpp ----------------------------===//
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
#include "pstore/support/small_vector.hpp"

#include <numeric>

#include "gmock/gmock.h"

TEST (SmallVector, DefaultCtor) {
    pstore::small_vector<int, 8> b;
    EXPECT_EQ (0U, b.size ()) << "expected the initial size to be number number of stack elements";
    EXPECT_EQ (8U, b.capacity ());
    EXPECT_TRUE (b.empty ());
}

TEST (SmallVector, ExplicitCtorLessThanStackBuffer) {
    pstore::small_vector<int, 8> const b (std::size_t{5});
    EXPECT_EQ (5U, b.size ());
    EXPECT_EQ (8U, b.capacity ());
    EXPECT_EQ (5U * sizeof (int), b.size_bytes ());
}

TEST (SmallVector, ExplicitCtor0) {
    pstore::small_vector<int, 8> const b (std::size_t{0});
    EXPECT_EQ (0U, b.size ());
    EXPECT_EQ (8U, b.capacity ());
    EXPECT_EQ (0U * sizeof (int), b.size_bytes ());
    EXPECT_TRUE (b.empty ());
}

TEST (SmallVector, ExplicitCtorGreaterThanStackBuffer) {
    pstore::small_vector<int, 8> const b (std::size_t{10});
    EXPECT_EQ (10U, b.size ());
    EXPECT_EQ (10U, b.capacity ());
    EXPECT_EQ (10 * sizeof (int), b.size_bytes ());
}

TEST (SmallVector, CtorInitializerList) {
    pstore::small_vector<int, 8> const b{1, 2, 3};
    EXPECT_EQ (3U, b.size ());
    EXPECT_EQ (8U, b.capacity ());
    EXPECT_THAT (b, ::testing::ElementsAre (1, 2, 3));
}

TEST (SmallVector, CtorCopy) {
    pstore::small_vector<int, 3> const b{3, 5};
    pstore::small_vector<int, 3> c = b;
    EXPECT_EQ (2U, c.size ());
    EXPECT_THAT (c, ::testing::ElementsAre (3, 5));
}

TEST (SmallVector, MoveCtor) {
    pstore::small_vector<int, 4> a (std::size_t{4});
    std::iota (a.begin (), a.end (), 0); // fill with increasing values
    pstore::small_vector<int, 4> const b (std::move (a));

    EXPECT_THAT (b, ::testing::ElementsAre (0, 1, 2, 3));
}

TEST (SmallVector, AssignInitializerList) {
    pstore::small_vector<int, 3> b{1, 2, 3};
    b.assign ({4, 5, 6, 7});
    EXPECT_THAT (b, ::testing::ElementsAre (4, 5, 6, 7));
}

TEST (SmallVector, AssignCopy) {
    pstore::small_vector<int, 3> const b{5, 7};
    pstore::small_vector<int, 3> c;
    c = b;
    EXPECT_THAT (c, ::testing::ElementsAre (5, 7));
}

TEST (SmallVector, SizeAfterResizeLarger) {
    pstore::small_vector<int, 4> b (std::size_t{4});
    std::size_t const size{10};
    b.resize (size);
    EXPECT_EQ (size, b.size ());
    EXPECT_GE (size, b.capacity ())
        << "expected capacity to be at least " << size << " (the container size)";
}

TEST (SmallVector, ContentsAfterResizeLarger) {
    constexpr auto orig_size = std::size_t{8};
    constexpr auto new_size = std::size_t{10};

    pstore::small_vector<int, orig_size> b (orig_size);
    std::iota (std::begin (b), std::end (b), 37);
    b.resize (new_size);
    ASSERT_EQ (b.size (), new_size);

    std::vector<int> actual;
    std::copy_n (std::begin (b), orig_size, std::back_inserter (actual));
    EXPECT_THAT (actual, ::testing::ElementsAre (37, 38, 39, 40, 41, 42, 43, 44));
}

TEST (SmallVector, SizeAfterResizeSmaller) {
    pstore::small_vector<int, 8> b (std::size_t{8});
    b.resize (5);
    EXPECT_EQ (5U, b.size ());
    EXPECT_EQ (8U, b.capacity ());
    EXPECT_FALSE (b.empty ());
}

TEST (SmallVector, SizeAfterResize0) {
    pstore::small_vector<int, 8> b (std::size_t{8});
    b.resize (0);
    EXPECT_EQ (0U, b.size ());
    EXPECT_EQ (8U, b.capacity ());
    EXPECT_TRUE (b.empty ());
}

TEST (SmallVector, DataAndConstDataMatch) {
    pstore::small_vector<int, 8> b (std::size_t{8});
    auto const * const bconst = &b;
    EXPECT_EQ (bconst->data (), b.data ());
}

TEST (SmallVector, IteratorNonConst) {
    pstore::small_vector<int, 4> buffer (std::size_t{4});

    // I populate the buffer manually here to ensure coverage of basic iterator
    // operations, but use std::iota() elsewhere to keep the tests simple.
    int value = 42;
    for (decltype (buffer)::iterator it = buffer.begin (), end = buffer.end (); it != end; ++it) {
        *it = value++;
    }

    {
        // Manually copy the contents of the buffer to a new vector.
        std::vector<int> actual;
        for (decltype (buffer)::iterator it = buffer.begin (), end = buffer.end (); it != end;
             ++it) {

            actual.push_back (*it);
        }
        EXPECT_THAT (actual, ::testing::ElementsAre (42, 43, 44, 45));
    }
}

TEST (SmallVector, IteratorConstFromNonConstContainer) {
    pstore::small_vector<int, 4> buffer (std::size_t{4});
    std::iota (buffer.begin (), buffer.end (), 42);

    {
        // Manually copy the contents of the buffer to a new vector but use a
        /// const iterator to do it this time.
        std::vector<int> actual;
        for (decltype (buffer)::const_iterator it = buffer.cbegin (), end = buffer.cend ();
             it != end; ++it) {

            actual.push_back (*it);
        }
        EXPECT_THAT (actual, ::testing::ElementsAre (42, 43, 44, 45));
    }
}

TEST (SmallVector, IteratorConstIteratorFromConstContainer) {
    pstore::small_vector<int, 4> buffer (std::size_t{4});
    std::iota (buffer.begin (), buffer.end (), 42);

    auto const & cbuffer = buffer;
    std::vector<int> const actual (cbuffer.begin (), cbuffer.end ());
    EXPECT_THAT (actual, ::testing::ElementsAre (42, 43, 44, 45));
}

TEST (SmallVector, IteratorNonConstReverse) {
    pstore::small_vector<int, 4> buffer (std::size_t{4});
    std::iota (buffer.begin (), buffer.end (), 42);

    {
        std::vector<int> const actual (buffer.rbegin (), buffer.rend ());
        EXPECT_THAT (actual, ::testing::ElementsAre (45, 44, 43, 42));
    }
    {
        std::vector<int> const actual (buffer.rcbegin (), buffer.rcend ());
        EXPECT_THAT (actual, ::testing::ElementsAre (45, 44, 43, 42));
    }
}

TEST (SmallVector, IteratorConstReverse) {
    // Wrap the buffer construction code in a lambda to hide the non-const
    // small_vector instance.
    auto const & cbuffer = []() {
        pstore::small_vector<int, 4> buffer (std::size_t{4});
        std::iota (std::begin (buffer), std::end (buffer), 42); // fill with increasing values
        return buffer;
    }();

    std::vector<int> actual (cbuffer.rbegin (), cbuffer.rend ());
    EXPECT_THAT (actual, ::testing::ElementsAre (45, 44, 43, 42));
}

TEST (SmallVector, ElementAccess) {
    pstore::small_vector<int, 4> buffer (std::size_t{4});
    int count = 42;
    for (std::size_t index = 0, end = buffer.size (); index != end; ++index) {
        buffer[index] = count++;
    }

    std::array<int, 4> const expected{{42, 43, 44, 45}};
    EXPECT_TRUE (std::equal (std::begin (buffer), std::end (buffer), std::begin (expected)));
}

TEST (SmallVector, MoveSmall) {
    pstore::small_vector<int, 4> a (std::size_t{3});
    pstore::small_vector<int, 4> b (std::size_t{4});
    std::fill (std::begin (a), std::end (a), 0);
    std::fill (std::begin (b), std::end (b), 73);

    a = std::move (b);
    EXPECT_THAT (a, ::testing::ElementsAre (73, 73, 73, 73));
}

TEST (SmallVector, MoveLarge) {
    // The two containers start out with different sizes; one uses the small
    // buffer, the other, large.
    pstore::small_vector<int, 3> a (std::size_t{0});
    pstore::small_vector<int, 3> b (std::size_t{4});
    std::fill (std::begin (a), std::end (a), 0);
    std::fill (std::begin (b), std::end (b), 73);
    a = std::move (b);

    EXPECT_THAT (a, ::testing::ElementsAre (73, 73, 73, 73));
}

TEST (SmallVector, Clear) {
    // The two containers start out with different sizes; one uses the small
    // buffer, the other, large.
    pstore::small_vector<int> a (std::size_t{4});
    EXPECT_EQ (4U, a.size ());
    a.clear ();
    EXPECT_EQ (0U, a.size ());
}

TEST (SmallVector, PushBack) {
    using ::testing::ElementsAre;
    pstore::small_vector<int, 2> a;
    a.push_back (1);
    EXPECT_THAT (a, ElementsAre (1));
    a.push_back (2);
    EXPECT_THAT (a, ElementsAre (1, 2));
    a.push_back (3);
    EXPECT_THAT (a, ElementsAre (1, 2, 3));
    a.push_back (4);
    EXPECT_THAT (a, ElementsAre (1, 2, 3, 4));
}

TEST (SmallVector, Back) {
    pstore::small_vector<int, 1> a;
    a.push_back (1);
    EXPECT_EQ (a.back (), 1);
    a.push_back (2);
    EXPECT_EQ (a.back (), 2);
}

TEST (SmallVector, AppendIteratorRange) {
    pstore::small_vector<int, 4> a (std::size_t{4});
    std::iota (std::begin (a), std::end (a), 0);

    std::array<int, 4> extra;
    std::iota (std::begin (extra), std::end (extra), 100);

    a.append (std::begin (extra), std::end (extra));

    EXPECT_THAT (a, ::testing::ElementsAre (0, 1, 2, 3, 100, 101, 102, 103));
}
