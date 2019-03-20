//*  _                     _              _    *
//* | |__   __ _ _ __ ___ | |_   ___  ___| |_  *
//* | '_ \ / _` | '_ ` _ \| __| / __|/ _ \ __| *
//* | | | | (_| | | | | | | |_  \__ \  __/ |_  *
//* |_| |_|\__,_|_| |_| |_|\__| |___/\___|\__| *
//*                                            *
//===- unittests/core/test_hamt_set.cpp -----------------------------------===//
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
#include "pstore/core/hamt_set.hpp"

#include <random>
#include "gtest/gtest.h"
#include "pstore/core/transaction.hpp"

#include "empty_store.hpp"
#include "mock_mutex.hpp"

//*  ___      _   ___ _     _                 *
//* / __| ___| |_| __(_)_ _| |_ _  _ _ _ ___  *
//* \__ \/ -_)  _| _|| \ \ /  _| || | '_/ -_) *
//* |___/\___|\__|_| |_/_\_\\__|\_,_|_| \___| *
//*                                           *

namespace {

    class SetFixture : public EmptyStore {
    public:
        void SetUp () override;
        void TearDown () override;

        using lock_guard = std::unique_lock<mock_mutex>;
        using transaction_type = pstore::transaction<lock_guard>;
        using set = pstore::index::hamt_set<std::string>;
        using iterator = set::iterator;
        using const_iterator = set::const_iterator;

    protected:
        mock_mutex mutex_;
        std::unique_ptr<pstore::database> db_;
        std::unique_ptr<set> index_;
    };

    // SetUp
    // ~~~~~
    void SetFixture::SetUp () {
        EmptyStore::SetUp ();
        db_.reset (new pstore::database (this->file ()));
        db_->set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        index_.reset (new set{*db_});
    }

    // TearDown
    // ~~~~~~~~
    void SetFixture::TearDown () {
        db_.reset ();
        EmptyStore::TearDown ();
    }
} // namespace

// test default constructor.
TEST_F (SetFixture, DefaultConstructor) {
    EXPECT_EQ (0U, index_->size ());
    EXPECT_TRUE (index_->empty ());
}

// test iterator: empty index.
TEST_F (SetFixture, EmptyBeginEqualsEnd) {
    iterator begin = index_->begin (*db_);
    iterator end = index_->end (*db_);
    EXPECT_EQ (begin, end);
    const_iterator cbegin = index_->cbegin (*db_);
    const_iterator cend = index_->cend (*db_);
    EXPECT_EQ (cbegin, cend);
}

// test insert: index only contains a single leaf node.
TEST_F (SetFixture, InsertSingleLeaf) {
    transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
    std::pair<iterator, bool> itp = index_->insert (t1, std::string{"a"});
    std::string const & key = (*itp.first);
    EXPECT_EQ ("a", key);
    EXPECT_TRUE (itp.second);
    itp = index_->insert (t1, std::string{"a"});
    EXPECT_FALSE (itp.second);
    EXPECT_EQ (1U, index_->size ());
}

// test find: index only contains a single leaf node.
TEST_F (SetFixture, FindSingle) {
    transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
    const_iterator cend = index_->cend (*db_);
    std::string const a{"a"};
    EXPECT_EQ (index_->find (*db_, a), cend);
    index_->insert (t1, a);
    auto it = index_->find (*db_, a);
    EXPECT_NE (it, cend);
    EXPECT_EQ (*it, a);
    index_->flush (t1, db_->get_current_revision());
    it = index_->find (*db_, a);
    EXPECT_NE (it, cend);
    EXPECT_EQ (*it, a);
}

// test iterator: index only contains a single leaf node.
TEST_F (SetFixture, InsertSingleIterator) {
    transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
    index_->insert (t1, std::string{"a"});

    iterator begin = index_->begin (*db_);
    iterator end = index_->end (*db_);
    EXPECT_NE (begin, end);
    std::string const & v1 = (*begin);
    EXPECT_EQ ("a", v1);
    ++begin;
    EXPECT_EQ (begin, end);
}

// test iterator: index contains an internal heap node.
TEST_F (SetFixture, InsertHeap) {
    transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
    index_->insert (t1, std::string{"a"});
    index_->insert (t1, std::string{"b"});
    EXPECT_EQ (2U, index_->size ());

    iterator begin = index_->begin (*db_);
    iterator end = index_->end (*db_);
    EXPECT_NE (begin, end);
    ++begin;
    EXPECT_NE (begin, end);
    ++begin;
    EXPECT_EQ (begin, end);
}

// test iterator: index only contains a leaf store node.
TEST_F (SetFixture, InsertLeafStore) {
    transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
    index_->insert (t1, std::string{"a"});
    index_->flush (t1, db_->get_current_revision());

    const_iterator begin = index_->cbegin (*db_);
    const_iterator end = index_->cend (*db_);
    EXPECT_NE (begin, end);
    std::string const & v1 = (*begin);
    EXPECT_EQ ("a", v1);
    begin++;
    EXPECT_EQ (begin, end);
}

// test iterator: index contains an internal store node.
TEST_F (SetFixture, InsertInternalStoreIterator) {
    transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
    index_->insert (t1, std::string{"a"});
    index_->insert (t1, std::string{"b"});
    index_->flush (t1, db_->get_current_revision());

    const_iterator begin = index_->cbegin (*db_);
    const_iterator end = index_->cend (*db_);
    EXPECT_NE (begin, end);
    begin++;
    EXPECT_NE (begin, end);
    begin++;
    EXPECT_EQ (begin, end);
}

// test insert: index contains an internal store node.
TEST_F (SetFixture, InsertInternalStore) {
    transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
    std::pair<iterator, bool> itp1 = index_->insert (t1, std::string{"a"});
    std::pair<iterator, bool> itp2 = index_->insert (t1, std::string{"b"});

    std::string const & key1 = (*itp1.first);
    EXPECT_EQ ("a", key1);
    EXPECT_TRUE (itp1.second);
    std::string const & key2 = (*itp2.first);
    EXPECT_EQ ("b", key2);
    EXPECT_TRUE (itp2.second);
    index_->flush (t1, db_->get_current_revision());

    std::pair<iterator, bool> itp3 = index_->insert (t1, std::string{"a"});
    EXPECT_FALSE (itp3.second);
}

// test find: index only contains a single leaf node.
TEST_F (SetFixture, FindInternal) {
    transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
    const_iterator cend = index_->cend (*db_);
    std::string ini ("Initial string");

    index_->insert (t1, std::string{"a"});
    index_->insert (t1, ini);
    auto it = index_->find (*db_, std::string{"a"});
    EXPECT_NE (it, cend);
    EXPECT_EQ (*it, std::string{"a"});
    it = index_->find (*db_, ini);
    EXPECT_NE (it, cend);
    EXPECT_EQ (*it, ini);

    index_->flush (t1, db_->get_current_revision ());

    it = index_->find (*db_, std::string{"a"});
    EXPECT_NE (it, cend);
    EXPECT_EQ (*it, std::string{"a"});
    it = index_->find (*db_, ini);
    EXPECT_NE (it, cend);
    EXPECT_EQ (*it, ini);
    EXPECT_EQ (it->size (), 14U); // Check operator ->
}
