//===- unittests/adt/test_chunked_sequence.cpp ----------------------------===//
//*       _                 _            _  *
//*   ___| |__  _   _ _ __ | | _____  __| | *
//*  / __| '_ \| | | | '_ \| |/ / _ \/ _` | *
//* | (__| | | | |_| | | | |   <  __/ (_| | *
//*  \___|_| |_|\__,_|_| |_|_|\_\___|\__,_| *
//*                                         *
//*                                            *
//*  ___  ___  __ _ _   _  ___ _ __   ___ ___  *
//* / __|/ _ \/ _` | | | |/ _ \ '_ \ / __/ _ \ *
//* \__ \  __/ (_| | |_| |  __/ | | | (_|  __/ *
//* |___/\___|\__, |\__,_|\___|_| |_|\___\___| *
//*              |_|                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/adt/chunked_sequence.hpp"

// standard library
#include <utility>

// pstore includes
#include "pstore/config/config.hpp"

// 3rd party
#include <gmock/gmock.h>

namespace {

    // A class which simply wraps and int and doesn't have a default ctor.
    class simple {
    public:
        constexpr explicit simple (int v) noexcept
                : v_{v} {}
        constexpr int get () const noexcept { return v_; }

    private:
        int v_;
    };

    // Limit the chunks to two elements each.
    using cvector_int = pstore::chunked_sequence<int, std::size_t{2}>;
    using cvector_simple = pstore::chunked_sequence<simple, std::size_t{2}>;

} // end anonymous namespace

TEST (ChunkedSequence, Init) {
    cvector_int cv;
    EXPECT_EQ (cv.size (), 0U);
    EXPECT_EQ (std::distance (std::begin (cv), std::end (cv)), 0);
}

TEST (ChunkedSequence, OneMember) {
    cvector_simple cv;
    cv.emplace_back (1);

    EXPECT_EQ (cv.size (), 1U);
    auto it = cv.begin ();
    EXPECT_EQ (it->get (), 1);
    ++it;
    EXPECT_EQ (it, cv.end ());
}

TEST (ChunckedVector, PushBack) {
    cvector_simple cv;
    simple const & a = cv.push_back (simple{17});
    simple const & b = cv.push_back (simple{19});
    simple const & c = cv.push_back (simple{23});
    EXPECT_EQ (cv.size (), 3U);
    EXPECT_EQ (a.get (), 17);
    EXPECT_EQ (b.get (), 19);
    EXPECT_EQ (c.get (), 23);
}

TEST (ChunckedVector, EmplaceBack) {
    cvector_simple cv;
    simple const & a = cv.emplace_back (17);
    simple const & b = cv.emplace_back (19);
    simple const & c = cv.emplace_back (23);
    EXPECT_EQ (cv.size (), 3U);
    EXPECT_EQ (a.get (), 17);
    EXPECT_EQ (b.get (), 19);
    EXPECT_EQ (c.get (), 23);
}

TEST (ChunckedVector, FrontAndBack) {
    cvector_int cv;
    cv.push_back (17);
    cv.push_back (19);
    cv.push_back (23);
    EXPECT_EQ (cv.front (), 17);
    EXPECT_EQ (cv.back (), 23);
}

TEST (ChunkedSequence, Swap) {
    cvector_int a, b;
    a.emplace_back (7);
    std::swap (a, b);
    EXPECT_EQ (a.size (), 0U);
    ASSERT_EQ (b.size (), 1U);
    EXPECT_EQ (b.front (), 7);
}

TEST (ChunkedSequence, Splice) {
    cvector_int a;
    a.emplace_back (7);

    cvector_int b;
    b.emplace_back (11);

    a.splice (std::move (b));
    EXPECT_THAT (a, testing::ElementsAre (7, 11));
}

TEST (ChunkedSequence, SpliceOntoEmpty) {
    {
        // Start with an empty CV and splice a populated CV onto it.
        cvector_int a;
        cvector_int b;
        b.emplace_back (11);

        a.splice (std::move (b));
        EXPECT_EQ (a.front (), 11);
        EXPECT_THAT (a, testing::ElementsAre (11));
    }
    {
        // Start with a populated CV and splice an empty CV onto it.
        cvector_int c;
        cvector_int d;
        c.emplace_back (13);

        c.splice (std::move (d));
        EXPECT_EQ (c.front (), 13);
        EXPECT_THAT (c, testing::ElementsAre (13));
    }
}

TEST (ChunkedSequence, Clear) {
    cvector_int a;
    a.emplace_back (7);
    a.clear ();
    EXPECT_THAT (a.size (), 0U);

    // Try appending after the clear.
    a.emplace_back (11);
    EXPECT_THAT (a.size (), 1U);
}

TEST (ChunkedSequence, IteratorAssign) {
    cvector_int cv;
    cv.emplace_back (7);
    cvector_int::iterator it = cv.begin ();
    cvector_int::iterator it2 = it;                     // non-const copy ctor from non-const
    it2 = it;                                           // non-const assign from non-const.
    cvector_int::const_iterator cit = it;               // const copy ctor from non-const.
    cvector_int::const_iterator cit2 = cit;             // copy ctor from const.
    cit2 = cit;                                         // assign from const.
    cit2 = it;                                          // assign from non-const.
    cvector_int::const_iterator cit3 = std::move (cit); // move ctor from const.
    EXPECT_EQ (*cit3, 7);
}

namespace {

    template <typename IteratorType>
    class CVIterator : public testing::Test {
    public:
        CVIterator () {
            cv_.emplace_back (2);
            cv_.emplace_back (3);
            cv_.emplace_back (5);
            cv_.emplace_back (7);
        }

    protected:
        cvector_int cv_;
    };

} // end anonymous namespace

using iterator_types = testing::Types<cvector_int::iterator, cvector_int::const_iterator>;
#ifdef PSTORE_IS_INSIDE_LLVM
TYPED_TEST_CASE (CVIterator, iterator_types);
#else
TYPED_TEST_SUITE (CVIterator, iterator_types, );
#endif

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

//  - An underscore '_' indicates uninitialized storage.
//  - An arrow '->' indicates the list of chunks.
TEST (ChunkedVectorResize, FillCurrentTailChunk) {
    cvector_int cv;
    cv.emplace_back (13);
    // Before the resize we have a single chunk:
    //     [ 13, _ ]
    // After it, we fill the tail chunk with default-initialized int:
    //    [ 13, 0 ]
    cv.resize (2U);
    EXPECT_EQ (cv.size (), 2U);
    EXPECT_EQ (cv.capacity (), 2U);

    cvector_int::const_iterator it = cv.begin ();
    EXPECT_EQ (*it, 13) << "Element 0 (chunk 0, index 0)";
    std::advance (it, 1);
    EXPECT_EQ (*it, 0) << "Element 0 (chunk 0, index 1)";
}

TEST (ChunkedVectorResize, FillInitialChunkAndPartialSecond) {
    cvector_int cv;
    cv.emplace_back (17);
    // Before the resize we have a single chunk:
    //     [ 17, _ ]
    // Extending this to three members will produce:
    //     [ 17, 0 ] -> [ 0, _ ]
    cv.resize (3U);
    EXPECT_EQ (cv.size (), 3U);
    EXPECT_EQ (cv.capacity (), 4U);

    cvector_int::const_iterator it = cv.begin ();
    EXPECT_EQ (*it, 17) << "Element 0 (chunk 0, index 0)";
    std::advance (it, 1);
    EXPECT_EQ (*it, 0) << "Element 1 (chunk 0, index 1)";
    std::advance (it, 1);
    EXPECT_EQ (*it, 0) << "Element 2 (chunk 1, index 0)";
}

TEST (ChunkedVectorResize, ResizeWholeChunkPlus1) {
    cvector_int cv;
    // Resize from 0 to 5 elements:
    //     [ 0, 0 ] -> [ 0, 0 ] -> [ 0, _ ]
    cv.resize (5U);
    EXPECT_EQ (cv.size (), 5U);
    EXPECT_EQ (cv.capacity (), 6U);

    cvector_int::const_iterator it = cv.begin ();
    EXPECT_EQ (*it, 0) << "Element 0 (chunk 0, index 0)";
    std::advance (it, 1);
    EXPECT_EQ (*it, 0) << "Element 1 (chunk 0, index 1)";
    std::advance (it, 1);
    EXPECT_EQ (*it, 0) << "Element 2 (chunk 1, index 0)";
    std::advance (it, 1);
    EXPECT_EQ (*it, 0) << "Element 3 (chunk 1, index 1)";
    std::advance (it, 1);
    EXPECT_EQ (*it, 0) << "Element 4 (chunk 1, index 1)";
}

TEST (ChunkedVectorResize, TwoElementsDownToOne) {
    cvector_int cv;
    cv.emplace_back (17);
    cv.emplace_back (19);
    // Before: [ 17, 19 ]
    // After: [ 17, _ ]
    cv.resize (1U);
    EXPECT_EQ (cv.size (), 1U);
    EXPECT_EQ (cv.capacity (), 2U);

    cvector_int::const_iterator it = cv.begin ();
    EXPECT_EQ (*it, 17) << "Element 0 (chunk 0, index 0)";
}

TEST (ChunkedVectorResize, TwoElementsDownToZero) {
    cvector_int cv;
    cv.emplace_back (17);
    cv.emplace_back (19);
    // Before: [ 17, 19 ]
    // After: [ _, _ ]
    cv.resize (0U);
    EXPECT_EQ (cv.size (), 0U);
    EXPECT_EQ (cv.capacity (), 2U) << "There is always at least one chunk";
}

TEST (ChunkedVectorResize, FiveElementsDownToOne) {
    cvector_int cv;
    cv.emplace_back (17);
    cv.emplace_back (19);
    cv.emplace_back (23);
    cv.emplace_back (29);
    cv.emplace_back (31);
    // Before: [ 17, 19 ] -> [ 23, 29 ] -> [ 31, _ ]
    // After: [ 17, _ ]
    cv.resize (1U);
    EXPECT_EQ (cv.size (), 1U);
    EXPECT_EQ (cv.capacity (), 2U);

    cvector_int::const_iterator it = cv.begin ();
    EXPECT_EQ (*it, 17) << "Element 0 (chunk 0, index 0)";
}

TEST (ChunkedVectorResize, ThreeElementsDownToZero) {
    cvector_int cv;
    cv.emplace_back (37);
    cv.emplace_back (41);
    cv.emplace_back (43);
    // Before: [ 37, 41 ] -> [ 43, _ ]
    // After: [ _, _ ]
    cv.resize (0U);
    EXPECT_EQ (cv.size (), 0U);
    EXPECT_EQ (cv.capacity (), 2U);
    EXPECT_TRUE (cv.empty ());
}
