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
    using cseq_int = pstore::chunked_sequence<int, std::size_t{2}>;
    using cseq_simple = pstore::chunked_sequence<simple, std::size_t{2}>;

} // end anonymous namespace

TEST (ChunkedSequence, Init) {
    cseq_int cs;
    EXPECT_EQ (cs.size (), 0U);
    EXPECT_EQ (std::distance (std::begin (cs), std::end (cs)), 0);
}

TEST (ChunkedSequence, OneMember) {
    cseq_simple cs;
    cs.emplace_back (1);

    EXPECT_EQ (cs.size (), 1U);
    auto it = cs.begin ();
    EXPECT_EQ (it->get (), 1);
    ++it;
    EXPECT_EQ (it, cs.end ());
}

TEST (ChunckedVector, PushBack) {
    cseq_simple cs;
    simple const & a = cs.push_back (simple{17});
    simple const & b = cs.push_back (simple{19});
    simple const & c = cs.push_back (simple{23});
    EXPECT_EQ (cs.size (), 3U);
    EXPECT_EQ (a.get (), 17);
    EXPECT_EQ (b.get (), 19);
    EXPECT_EQ (c.get (), 23);
}

TEST (ChunckedVector, EmplaceBack) {
    cseq_simple cs;
    simple const & a = cs.emplace_back (17);
    simple const & b = cs.emplace_back (19);
    simple const & c = cs.emplace_back (23);
    EXPECT_EQ (cs.size (), 3U);
    EXPECT_EQ (a.get (), 17);
    EXPECT_EQ (b.get (), 19);
    EXPECT_EQ (c.get (), 23);
}

TEST (ChunckedVector, FrontAndBack) {
    cseq_int cs;
    cs.push_back (17);
    cs.push_back (19);
    cs.push_back (23);
    EXPECT_EQ (cs.front (), 17);
    EXPECT_EQ (cs.back (), 23);
}

TEST (ChunkedSequence, Swap) {
    cseq_int a, b;
    a.emplace_back (7);
    std::swap (a, b);
    EXPECT_EQ (a.size (), 0U);
    ASSERT_EQ (b.size (), 1U);
    EXPECT_EQ (b.front (), 7);
}

TEST (ChunkedSequence, Splice) {
    cseq_int a;
    a.emplace_back (7);

    cseq_int b;
    b.emplace_back (11);

    a.splice (std::move (b));
    EXPECT_THAT (a, testing::ElementsAre (7, 11));
}

TEST (ChunkedSequence, SpliceOntoEmpty) {
    {
        // Start with an empty CS and splice a populated CS onto it.
        cseq_int a;
        cseq_int b;
        b.emplace_back (11);

        a.splice (std::move (b));
        EXPECT_EQ (a.front (), 11);
        EXPECT_THAT (a, testing::ElementsAre (11));
    }
    {
        // Start with a populated CS and splice an empty CS onto it.
        cseq_int c;
        cseq_int d;
        c.emplace_back (13);

        c.splice (std::move (d));
        EXPECT_EQ (c.front (), 13);
        EXPECT_THAT (c, testing::ElementsAre (13));
    }
}

TEST (ChunkedSequence, Clear) {
    cseq_int a;
    a.emplace_back (7);
    a.clear ();
    EXPECT_THAT (a.size (), 0U);

    // Try appending after the clear.
    a.emplace_back (11);
    EXPECT_THAT (a.size (), 1U);
}

TEST (ChunkedSequence, IteratorAssign) {
    cseq_int cs;
    cs.emplace_back (7);
    cseq_int::iterator it = cs.begin ();
    cseq_int::iterator it2 = it;                     // non-const copy ctor from non-const
    it2 = it;                                        // non-const assign from non-const.
    cseq_int::const_iterator cit = it;               // const copy ctor from non-const.
    cseq_int::const_iterator cit2 = cit;             // copy ctor from const.
    cit2 = cit;                                      // assign from const.
    cit2 = it;                                       // assign from non-const.
    cseq_int::const_iterator cit3 = std::move (cit); // move ctor from const.
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
        cseq_int cv_;
    };

} // end anonymous namespace

using iterator_types = testing::Types<cseq_int::iterator, cseq_int::const_iterator>;
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
TEST (ChunkedSequenceResize, FillCurrentTailChunk) {
    cseq_int cs;
    cs.emplace_back (13);
    // Before the resize we have a single chunk:
    //     [ 13, _ ]
    EXPECT_EQ (cs.chunks_size (), 1U);
    // After it, we fill the tail chunk with default-initialized int:
    //    [ 13, 0 ]
    cs.resize (2U);
    EXPECT_EQ (cs.chunks_size (), 1U);
    EXPECT_EQ (cs.size (), 2U);
    EXPECT_EQ (cs.capacity (), 2U);

    cseq_int::const_iterator it = cs.begin ();
    EXPECT_EQ (*it, 13) << "Element 0 (chunk 0, index 0)";
    std::advance (it, 1);
    EXPECT_EQ (*it, 0) << "Element 0 (chunk 0, index 1)";
}

TEST (ChunkedSequenceResize, FillInitialChunkAndPartialSecond) {
    cseq_int cs;
    cs.emplace_back (17);
    // Before the resize we have a single chunk:
    //     [ 17, _ ]
    EXPECT_EQ (cs.chunks_size (), 1U);
    // Extending this to three members will produce:
    //     [ 17, 0 ] -> [ 0, _ ]
    cs.resize (3U);
    EXPECT_EQ (cs.chunks_size (), 2U);
    EXPECT_EQ (cs.size (), 3U);
    EXPECT_EQ (cs.capacity (), 4U);

    cseq_int::const_iterator it = cs.begin ();
    EXPECT_EQ (*it, 17) << "Element 0 (chunk 0, index 0)";
    std::advance (it, 1);
    EXPECT_EQ (*it, 0) << "Element 1 (chunk 0, index 1)";
    std::advance (it, 1);
    EXPECT_EQ (*it, 0) << "Element 2 (chunk 1, index 0)";
}

TEST (ChunkedSequenceResize, ResizeWholeChunkPlus1) {
    cseq_int cs;
    // Resize from 0 to 5 elements:
    //     [ 0, 0 ] -> [ 0, 0 ] -> [ 0, _ ]
    EXPECT_EQ (cs.chunks_size (), 1U);
    cs.resize (5U);
    EXPECT_EQ (cs.chunks_size (), 3U);
    EXPECT_EQ (cs.size (), 5U);
    EXPECT_EQ (std::distance (std::begin (cs), std::end (cs)), 5U);
    EXPECT_EQ (cs.capacity (), 6U);

    cseq_int::const_iterator it = cs.begin ();
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

TEST (ChunkedSequenceResize, TwoElementsDownToOne) {
    cseq_int cs;
    cs.emplace_back (17);
    cs.emplace_back (19);
    // Before: [ 17, 19 ]
    EXPECT_EQ (cs.chunks_size (), 1U);
    // After: [ 17, _ ]
    cs.resize (1U);
    EXPECT_EQ (cs.chunks_size (), 1U);
    EXPECT_EQ (cs.size (), 1U);
    EXPECT_EQ (std::distance (std::begin (cs), std::end (cs)), 1U);
    EXPECT_EQ (cs.capacity (), 2U);

    cseq_int::const_iterator it = cs.begin ();
    EXPECT_EQ (*it, 17) << "Element 0 (chunk 0, index 0)";
}

TEST (ChunkedSequenceResize, TwoElementsDownToZero) {
    cseq_int cs;
    cs.emplace_back (17);
    cs.emplace_back (19);
    // Before: [ 17, 19 ]
    EXPECT_EQ (cs.chunks_size (), 1U);
    // After: [ _, _ ]
    cs.resize (0U);
    EXPECT_EQ (cs.chunks_size (), 1U);
    EXPECT_EQ (cs.size (), 0U);
    EXPECT_EQ (std::distance (std::begin (cs), std::end (cs)), 0U);
    EXPECT_EQ (cs.capacity (), 2U) << "There is always at least one chunk";
}

TEST (ChunkedSequenceResize, FiveElementsDownToOne) {
    cseq_int cs;
    cs.emplace_back (17);
    cs.emplace_back (19);
    cs.emplace_back (23);
    cs.emplace_back (29);
    cs.emplace_back (31);
    // Before: [ 17, 19 ] -> [ 23, 29 ] -> [ 31, _ ]
    EXPECT_EQ (cs.chunks_size (), 3U);
    // After: [ 17, _ ]
    cs.resize (1U);
    EXPECT_EQ (cs.chunks_size (), 1U);
    EXPECT_EQ (cs.size (), 1U);
    EXPECT_EQ (std::distance (std::begin (cs), std::end (cs)), 1U);
    EXPECT_EQ (cs.capacity (), 2U);

    cseq_int::const_iterator it = cs.begin ();
    EXPECT_EQ (*it, 17) << "Element 0 (chunk 0, index 0)";
}

TEST (ChunkedSequenceResize, ThreeElementsDownToZero) {
    cseq_int cs;
    cs.emplace_back (37);
    cs.emplace_back (41);
    cs.emplace_back (43);
    // Before: [ 37, 41 ] -> [ 43, _ ]
    EXPECT_EQ (cs.chunks_size (), 2U);
    // After: [ _, _ ]
    cs.resize (0U);
    EXPECT_EQ (cs.chunks_size (), 1U);
    EXPECT_EQ (cs.size (), 0U);
    EXPECT_EQ (std::distance (std::begin (cs), std::end (cs)), 0U);
    EXPECT_EQ (cs.capacity (), 2U);
    EXPECT_TRUE (cs.empty ());
}

TEST (ChunkedSequenceResize, ThreeElementsDownToOne) {
    pstore::chunked_sequence<int, 3U> cs;
    using difference_type = std::iterator_traits<decltype (cs)::iterator>::difference_type;
    {
        constexpr auto size1 = std::size_t{3};
        cs.resize (size1);
        EXPECT_EQ (cs.size (), size1);
        EXPECT_EQ (std::distance (std::begin (cs), std::end (cs)),
                   static_cast<difference_type> (size1));
        EXPECT_TRUE (std::all_of (std::begin (cs), std::end (cs), [] (int x) { return x == 0; }));
    }
    {
        constexpr auto size2 = std::size_t{1};
        cs.resize (size2);
        EXPECT_EQ (cs.size (), size2);
        EXPECT_EQ (std::distance (std::begin (cs), std::end (cs)),
                   static_cast<difference_type> (size2));
        EXPECT_TRUE (std::all_of (std::begin (cs), std::end (cs), [] (int x) { return x == 0; }));
    }
}

TEST (ChunkedSequenceChunkIterators, Empty) {
    pstore::chunked_sequence<int, 2U> cs;
    EXPECT_EQ (cs.chunks_size (), 1U) << "A chunked sequence has at least one chunk";
    decltype (cs)::chunk_list::iterator it = cs.chunks_begin ();
    EXPECT_EQ (it->size (), 0U);
    std::advance (it, 1);
    EXPECT_EQ (it, cs.chunks_end ());
}

TEST (ChunkedSequenceChunkIterators, TwoChunks) {
    pstore::chunked_sequence<int, 2U> cs;

    cs.emplace_back (47);
    cs.emplace_back (53);
    cs.emplace_back (59);

    EXPECT_EQ (cs.chunks_size (), 2U);
    decltype (cs)::chunk_list::iterator it = cs.chunks_begin ();
    EXPECT_EQ (it->size (), 2U);
    std::advance (it, 1);
    EXPECT_EQ (it->size (), 1U);
    std::advance (it, 1);
    EXPECT_EQ (it, cs.chunks_end ());
}
