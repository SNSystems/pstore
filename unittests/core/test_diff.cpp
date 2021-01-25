//*      _ _  __  __  *
//*   __| (_)/ _|/ _| *
//*  / _` | | |_| |_  *
//* | (_| | |  _|  _| *
//*  \__,_|_|_| |_|   *
//*                   *
//===- unittests/core/test_diff.cpp ---------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/core/diff.hpp"

#include <functional>
#include <memory>
#include <mutex>

#include "gmock/gmock.h"

#include "pstore/core/index_types.hpp"

#include "empty_store.hpp"
#include "split.hpp"

namespace {

    class Diff : public EmptyStore {
    public:
        void SetUp () override;
        void TearDown () override;

        using lock_guard = std::unique_lock<mock_mutex>;
        using transaction_type = pstore::transaction<lock_guard>;

        pstore::extent<char> add (transaction_type & transaction, std::string const & key,
                                  std::string const & value);

    protected:
        mock_mutex mutex_;
        std::unique_ptr<pstore::database> db_;
    };

    // SetUp
    // ~~~~~
    void Diff::SetUp () {
        EmptyStore::SetUp ();
        db_.reset (new pstore::database (this->file ()));
        db_->set_vacuum_mode (pstore::database::vacuum_mode::disabled);
    }

    // TearDown
    // ~~~~~~~~
    void Diff::TearDown () {
        db_.reset ();
        EmptyStore::TearDown ();
    }

    // add
    // ~~~
    pstore::extent<char> Diff::add (transaction_type & transaction, std::string const & key,
                                    std::string const & value) {
        auto where = pstore::typed_address<char>::null ();
        {
            // Allocate storage for string 'value' and copy the data into it.
            std::shared_ptr<char> ptr;
            std::tie (ptr, where) = transaction.alloc_rw<char> (value.length ());
            std::copy (std::begin (value), std::end (value), ptr.get ());
        }

        auto index = pstore::index::get_index<pstore::trailer::indices::write> (*db_);
        auto const value_extent = make_extent (where, value.length ());
        index->insert_or_assign (transaction, key, value_extent);
        return value_extent;
    }

} // namespace

namespace {

    template <typename Index, typename AddressIterator>
    std::vector<typename Index::value_type>
    addresses_to_values (pstore::database const & db, Index const & index, AddressIterator first,
                         AddressIterator last) {
        std::vector<typename Index::value_type> values;
        std::transform (
            first, last, std::back_inserter (values),
            [&db, &index] (pstore::address addr) { return index.load_leaf_node (db, addr); });
        return values;
    }

} // namespace

TEST_F (Diff, BuildWriteIndexValues) {
    using ::testing::ContainerEq;
    using ::testing::WhenSortedBy;

    using value_type = std::pair<std::string, pstore::extent<char>>;
    value_type v1, v2;

    {
        std::string const k1 = "key1";
        transaction_type t1 = begin (*db_, lock_guard{mutex_});
        v1 = std::make_pair (k1, this->add (t1, k1, "first value"));
        t1.commit ();
    }
    {
        std::string const k2 = "key2";
        transaction_type t2 = begin (*db_, lock_guard{mutex_});
        v2 = std::make_pair (k2, this->add (t2, k2, "second value"));
        t2.commit ();
    }
    ASSERT_EQ (2U, db_->get_current_revision ());

    {
        // Check the diff between r2 and r0.
        auto index = pstore::index::get_index<pstore::trailer::indices::write> (*db_);
        ASSERT_NE (index, nullptr);

        std::vector<pstore::address> actual;
        pstore::diff (*db_, *index, 0U, std::back_inserter (actual));
        auto const actual_values =
            addresses_to_values (*db_, *index, std::begin (actual), std::end (actual));
        EXPECT_THAT (actual_values, ::testing::UnorderedElementsAre (v1, v2));
    }

    {
        // Check the diff between r2 and r1.
        auto index = pstore::index::get_index<pstore::trailer::indices::write> (*db_);
        ASSERT_NE (index, nullptr);

        std::vector<pstore::address> actual;
        pstore::diff (*db_, *index, 1U, std::back_inserter (actual));
        auto const actual_values =
            addresses_to_values (*db_, *index, std::begin (actual), std::end (actual));
        EXPECT_THAT (actual_values, ::testing::UnorderedElementsAre (v2));
    }

    {
        // Check the diff between r2 and r2.
        std::vector<pstore::address> actual;
        pstore::diff (*db_, *pstore::index::get_index<pstore::trailer::indices::write> (*db_),
                            2U, std::back_inserter (actual));
        EXPECT_EQ (actual.size (), 0U);
    }
}

TEST_F (Diff, UncomittedTransaction) {
    using ::testing::UnorderedElementsAre;

    using value_type = std::pair<std::string, pstore::extent<char>>;
    value_type v1;

    {
        std::string const k1 = "key1";
        transaction_type t1 = begin (*db_, lock_guard{mutex_});
        v1 = std::make_pair (k1, this->add (t1, k1, "first value"));
        t1.commit ();
    }

    // The transaction t2 is left uncommitted whilst we perform the diff.
    std::string const k2 = "key2";
    transaction_type t2 = begin (*db_, lock_guard{mutex_});
    value_type v2 = std::make_pair (k2, this->add (t2, k2, "second value"));

    {
        // Check the diff between now (the uncommitted r2) and r0.
        auto index = pstore::index::get_index<pstore::trailer::indices::write> (*db_);
        ASSERT_NE (index, nullptr);

        std::vector<pstore::address> actual;
        pstore::diff (*db_, *index, 0U, std::back_inserter (actual));
        auto const actual_values =
            addresses_to_values (*db_, *index, std::begin (actual), std::end (actual));
        EXPECT_THAT (actual_values, UnorderedElementsAre (v1, v2));
    }

    {
        // Check the diff between now and r1.
        auto index = pstore::index::get_index<pstore::trailer::indices::write> (*db_);
        ASSERT_NE (index, nullptr);

        std::vector<pstore::address> actual;
        pstore::diff (*db_, *index, 1U, std::back_inserter (actual));
        auto const actual_values =
            addresses_to_values (*db_, *index, std::begin (actual), std::end (actual));
        EXPECT_THAT (actual_values, UnorderedElementsAre (v2));
    }

    {
        // Note that get_current_revision() still reports 1 even though a transaction is open.
        EXPECT_EQ (db_->get_current_revision (), 1U);
        std::vector<pstore::address> actual;
        pstore::diff (*db_, *pstore::index::get_index<pstore::trailer::indices::write> (*db_),
                            2U, std::back_inserter (actual));
        EXPECT_EQ (actual.size (), 0U);
    }

    t2.commit ();
}
