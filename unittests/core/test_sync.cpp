//*                        *
//*  ___ _   _ _ __   ___  *
//* / __| | | | '_ \ / __| *
//* \__ \ |_| | | | | (__  *
//* |___/\__, |_| |_|\___| *
//*      |___/             *
//===- unittests/core/test_sync.cpp ---------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#include "pstore/core/transaction.hpp"

#include <limits>
#include <memory>
#include <mutex>
#include <vector>

// 3rd party includes
#include "gmock/gmock.h"

// pstore
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/index_types.hpp"

// local includes
#include "check_for_error.hpp"
#include "empty_store.hpp"

namespace {
    class SyncFixture : public EmptyStore {
    public:
        void SetUp () override;
        void TearDown () override;

        using lock_guard = std::unique_lock<mock_mutex>;
        using transaction_type = pstore::transaction<lock_guard>;

        void add (transaction_type & transaction, std::string const & key,
                  std::string const & value);
        bool is_found (std::string const & key);
        void read (std::string const & key, std::string * const value_out);

    protected:
        mock_mutex mutex_;
        std::unique_ptr<pstore::database> db_;
    };

    // SetUp
    // ~~~~~
    void SyncFixture::SetUp () {
        EmptyStore::SetUp ();
        db_.reset (new pstore::database (this->file ()));
        db_->set_vacuum_mode (pstore::database::vacuum_mode::disabled);
    }

    // TearDown
    // ~~~~~~~~
    void SyncFixture::TearDown () {
        db_.reset ();
        EmptyStore::TearDown ();
    }

    // add
    // ~~~
    void SyncFixture::add (transaction_type & transaction, std::string const & key,
                           std::string const & value) {

        auto where = pstore::typed_address<char>::null ();
        {
            // Allocate storage for string 'value' and copy the data into it.
            std::shared_ptr<char> ptr;
            std::tie (ptr, where) = transaction.alloc_rw<char> (value.length ());
            std::copy (std::begin (value), std::end (value), ptr.get ());
        }

        auto index = pstore::index::get_index<pstore::trailer::indices::write> (*db_);
        index->insert_or_assign (transaction, key, make_extent (where, value.length ()));
    }

    // find
    // ~~~~
    bool SyncFixture::is_found (std::string const & key) {
        return pstore::index::get_index<pstore::trailer::indices::write> (*db_)->contains (*db_,
                                                                                           key);
    }

    // read
    // ~~~~
    void SyncFixture::read (std::string const & key, std::string * value_out) {
        auto index = pstore::index::get_index<pstore::trailer::indices::write> (*db_);
        auto const it = index->find (*db_, key);
        ASSERT_NE (it, index->cend (*db_));

        pstore::extent<char> const & r = it->second;
        std::shared_ptr<char const> value = db_->getro (r);
        *value_out = std::string{value.get (), r.size};
    }
} // namespace


TEST_F (SyncFixture, SyncBetweenVersions) {

    {
        transaction_type t1 = begin (*db_, lock_guard{mutex_});
        this->add (t1, "key0", "doesn't change");
        this->add (t1, "key1", "first value");
        t1.commit ();
    }
    {
        transaction_type t2 = begin (*db_, lock_guard{mutex_});
        this->add (t2, "key1", "second value");
        t2.commit ();
    }

    std::string value;

    this->read ("key1", &value);
    EXPECT_THAT (value, ::testing::StrEq ("second value"));
    this->read ("key0", &value);
    EXPECT_THAT (value, ::testing::StrEq ("doesn't change"));

    db_->sync (0);
    EXPECT_TRUE (db_->get_current_revision () == 0) << "The current revision should be 0";
    EXPECT_FALSE (this->is_found ("key0")) << "key0 should not be present at revision 0";
    EXPECT_FALSE (this->is_found ("key1")) << "key1 should not be present at revision 0";

    db_->sync (1);
    EXPECT_TRUE (db_->get_current_revision () == 1) << "The current revision should be 1";
    this->read ("key1", &value);
    EXPECT_THAT (value, ::testing::StrEq ("first value"));
    this->read ("key0", &value);
    EXPECT_THAT (value, ::testing::StrEq ("doesn't change"));

    db_->sync (2);
    EXPECT_TRUE (db_->get_current_revision () == 2) << "The current revision should be 2";
    this->read ("key1", &value);
    EXPECT_THAT (value, ::testing::StrEq ("second value"));
    this->read ("key0", &value);
    EXPECT_THAT (value, ::testing::StrEq ("doesn't change"));

    db_->sync (1);
    EXPECT_TRUE (db_->get_current_revision () == 1) << "The current revision should be 1";
    this->read ("key1", &value);
    EXPECT_THAT (value, ::testing::StrEq ("first value"));
    this->read ("key0", &value);
    EXPECT_THAT (value, ::testing::StrEq ("doesn't change"));
}

TEST_F (SyncFixture, SyncToBadVersions) {

    check_for_error ([this] () { db_->sync (1); }, pstore::error_code::unknown_revision);

    {
        transaction_type t1 = begin (*db_, lock_guard{mutex_});
        this->add (t1, "a", "first value");
        t1.commit ();
    }
    db_->sync (1);
    {
        transaction_type t2 = begin (*db_, lock_guard{mutex_});
        this->add (t2, "b", "second value");
        t2.commit ();
    }

    check_for_error ([this] () { db_->sync (3); }, pstore::error_code::unknown_revision);
}
