//===- unittests/core/test_hamt_set.cpp -----------------------------------===//
//*  _                     _              _    *
//* | |__   __ _ _ __ ___ | |_   ___  ___| |_  *
//* | '_ \ / _` | '_ ` _ \| __| / __|/ _ \ __| *
//* | | | | (_| | | | | | | |_  \__ \  __/ |_  *
//* |_| |_|\__,_|_| |_| |_|\__| |___/\___|\__| *
//*                                            *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/core/hamt_set.hpp"
// Standard library includes
#include <random>
// 3rd party includes
#include <gtest/gtest.h>
// pstore includes
#include "pstore/core/transaction.hpp"
// local includes
#include "empty_store.hpp"

using namespace std::string_literals;

//*  ___      _   ___ _     _                 *
//* / __| ___| |_| __(_)_ _| |_ _  _ _ _ ___  *
//* \__ \/ -_)  _| _|| \ \ /  _| || | '_/ -_) *
//* |___/\___|\__|_| |_/_\_\\__|\_,_|_| \___| *
//*                                           *

namespace {

    class SetFixture : public testing::Test {
    public:
        SetFixture ()
                : db_{store_.file ()}
                , index_{db_} {}

        using lock_guard = std::unique_lock<mock_mutex>;
        using transaction_type = pstore::transaction<lock_guard>;
        using set = pstore::index::hamt_set<std::string>;
        using iterator = set::iterator;
        using const_iterator = set::const_iterator;

    protected:
        mock_mutex mutex_;
        InMemoryStore store_;
        pstore::database db_;
        set index_;
    };

} // end anonymous namespace

// test default constructor.
TEST_F (SetFixture, DefaultConstructor) {
    EXPECT_EQ (0U, index_.size ());
    EXPECT_TRUE (index_.empty ());
}

// test iterator: empty index.
TEST_F (SetFixture, EmptyBeginEqualsEnd) {
    iterator begin = index_.begin (db_);
    iterator end = index_.end (db_);
    EXPECT_EQ (begin, end);
    const_iterator cbegin = index_.cbegin (db_);
    const_iterator cend = index_.cend (db_);
    EXPECT_EQ (cbegin, cend);
}

// test insert: index only contains a single leaf node.
TEST_F (SetFixture, InsertSingleLeaf) {
    transaction_type t1 = begin (db_, lock_guard{mutex_});
    std::pair<iterator, bool> itp = index_.insert (t1, "a"s);
    std::string const & key = (*itp.first);
    EXPECT_EQ ("a", key);
    EXPECT_TRUE (itp.second);
    itp = index_.insert (t1, "a"s);
    EXPECT_FALSE (itp.second);
    EXPECT_EQ (1U, index_.size ());
}

// test find: index only contains a single leaf node.
TEST_F (SetFixture, FindSingle) {
    transaction_type t1 = begin (db_, lock_guard{mutex_});
    const_iterator cend = index_.cend (db_);
    std::string const a{"a"};
    EXPECT_EQ (index_.find (db_, a), cend);
    index_.insert (t1, a);
    auto it = index_.find (db_, a);
    EXPECT_NE (it, cend);
    EXPECT_EQ (*it, a);
    index_.flush (t1, db_.get_current_revision ());
    it = index_.find (db_, a);
    EXPECT_NE (it, cend);
    EXPECT_EQ (*it, a);
}

// test iterator: index only contains a single leaf node.
TEST_F (SetFixture, InsertSingleIterator) {
    transaction_type t1 = begin (db_, lock_guard{mutex_});
    index_.insert (t1, "a"s);

    iterator begin = index_.begin (db_);
    iterator end = index_.end (db_);
    EXPECT_NE (begin, end);
    std::string const & v1 = (*begin);
    EXPECT_EQ ("a", v1);
    ++begin;
    EXPECT_EQ (begin, end);
}

// test iterator: index contains an internal heap node.
TEST_F (SetFixture, InsertHeap) {
    transaction_type t1 = begin (db_, lock_guard{mutex_});
    index_.insert (t1, "a"s);
    index_.insert (t1, "b"s);
    EXPECT_EQ (2U, index_.size ());

    iterator begin = index_.begin (db_);
    iterator end = index_.end (db_);
    EXPECT_NE (begin, end);
    ++begin;
    EXPECT_NE (begin, end);
    ++begin;
    EXPECT_EQ (begin, end);
}

// test iterator: index only contains a leaf store node.
TEST_F (SetFixture, InsertLeafStore) {
    transaction_type t1 = begin (db_, lock_guard{mutex_});
    index_.insert (t1, "a"s);
    index_.flush (t1, db_.get_current_revision ());

    const_iterator begin = index_.cbegin (db_);
    const_iterator end = index_.cend (db_);
    EXPECT_NE (begin, end);
    std::string const & v1 = (*begin);
    EXPECT_EQ ("a", v1);
    begin++;
    EXPECT_EQ (begin, end);
}

// test iterator: index contains an internal store node.
TEST_F (SetFixture, InsertInternalStoreIterator) {
    transaction_type t1 = begin (db_, lock_guard{mutex_});
    index_.insert (t1, "a"s);
    index_.insert (t1, "b"s);
    index_.flush (t1, db_.get_current_revision ());

    const_iterator begin = index_.cbegin (db_);
    const_iterator end = index_.cend (db_);
    EXPECT_NE (begin, end);
    begin++;
    EXPECT_NE (begin, end);
    begin++;
    EXPECT_EQ (begin, end);
}

// test insert: index contains an internal store node.
TEST_F (SetFixture, InsertInternalStore) {
    transaction_type t1 = begin (db_, lock_guard{mutex_});
    std::pair<iterator, bool> itp1 = index_.insert (t1, "a"s);
    std::pair<iterator, bool> itp2 = index_.insert (t1, "b"s);

    std::string const & key1 = (*itp1.first);
    EXPECT_EQ ("a", key1);
    EXPECT_TRUE (itp1.second);
    std::string const & key2 = (*itp2.first);
    EXPECT_EQ ("b", key2);
    EXPECT_TRUE (itp2.second);
    index_.flush (t1, db_.get_current_revision ());

    std::pair<iterator, bool> itp3 = index_.insert (t1, "a"s);
    EXPECT_FALSE (itp3.second);
}

// test find: index only contains a single leaf node.
TEST_F (SetFixture, FindInternal) {
    transaction_type t1 = begin (db_, lock_guard{mutex_});
    const_iterator cend = index_.cend (db_);
    std::string ini ("Initial string");

    index_.insert (t1, "a"s);
    index_.insert (t1, ini);
    auto it = index_.find (db_, "a"s);
    EXPECT_NE (it, cend);
    EXPECT_EQ (*it, "a"s);
    it = index_.find (db_, ini);
    EXPECT_NE (it, cend);
    EXPECT_EQ (*it, ini);

    index_.flush (t1, db_.get_current_revision ());

    it = index_.find (db_, "a"s);
    EXPECT_NE (it, cend);
    EXPECT_EQ (*it, "a"s);
    it = index_.find (db_, ini);
    EXPECT_NE (it, cend);
    EXPECT_EQ (*it, ini);
    EXPECT_EQ (it->size (), 14U); // Check operator ->
}
