//*      _ _  __  __  *
//*   __| (_)/ _|/ _| *
//*  / _` | | |_| |_  *
//* | (_| | |  _|  _| *
//*  \__,_|_|_| |_|   *
//*                   *
//===- unittests/diff/test_diff.cpp ---------------------------------------===//
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
#include "pstore/diff/diff.hpp"

#include <functional>
#include <memory>
#include <mutex>

#include "gmock/gmock.h"

#include "pstore/core/index_types.hpp"

#include "empty_store.hpp"
#include "mock_mutex.hpp"
#include "split.hpp"

namespace {

    class Diff : public EmptyStore {
    public:
        void SetUp () override;
        void TearDown () override;

        using lock_guard = std::unique_lock<mock_mutex>;
        using transaction_type = pstore::transaction<lock_guard>;

        pstore::extent add (transaction_type & transaction, std::string const & key,
                            std::string const & value);

    protected:
        mock_mutex mutex_;
        std::unique_ptr<pstore::database> db_;
    };

    // SetUp
    // ~~~~~
    void Diff::SetUp () {
        EmptyStore::SetUp ();
        db_.reset (new pstore::database (file_));
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
    pstore::extent Diff::add (transaction_type & transaction, std::string const & key,
                              std::string const & value) {
        auto where = pstore::typed_address<char>::null ();
        {
            // Allocate storage for string 'value' and copy the data into it.
            std::shared_ptr<char> ptr;
            std::tie (ptr, where) = transaction.alloc_rw<char> (value.length ());
            std::copy (std::begin (value), std::end (value), ptr.get ());
        }

        auto index = pstore::index::get_write_index (*db_);
        auto const value_extent = pstore::extent{where.to_address (), value.length ()};
        index->insert_or_assign (transaction, key, value_extent);
        return value_extent;
    }

} // namespace

namespace {

    template <typename Index, typename AddressIterator>
    std::vector<typename Index::value_type>
    addresses_to_values (Index const & index, AddressIterator first, AddressIterator last) {
        std::vector<typename Index::value_type> values;
        std::transform (first, last, std::back_inserter (values),
                        [&index](pstore::address addr) { return index.load_leaf_node (addr); });
        return values;
    }

} // namespace

TEST_F (Diff, BuildWriteIndexValues) {
    using ::testing::ContainerEq;
    using ::testing::WhenSortedBy;

    using value_type = std::pair<std::string, pstore::extent>;
    value_type v1, v2;

    {
        std::string const k1 = "key1";
        transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
        v1 = std::make_pair (k1, this->add (t1, k1, "first value"));
        t1.commit ();
    }
    {
        std::string const k2 = "key2";
        transaction_type t2 = pstore::begin (*db_, lock_guard{mutex_});
        v2 = std::make_pair (k2, this->add (t2, k2, "second value"));
        t2.commit ();
    }
    ASSERT_EQ (2U, db_->get_current_revision ());

    {
        // Check the diff between r2 and r0.
        pstore::diff::result_type actual =
            pstore::diff::diff (*pstore::index::get_write_index (*db_), 0U);
        auto index = pstore::index::get_write_index (*db_);
        ASSERT_NE (index, nullptr);
        auto const actual_values =
            addresses_to_values (*index, std::begin (actual), std::end (actual));
        EXPECT_THAT (actual_values, ::testing::UnorderedElementsAre (v1, v2));
    }

    {
        // Check the diff between r2 and r1.
        pstore::diff::result_type actual =
            pstore::diff::diff (*pstore::index::get_write_index (*db_), 1U);
        auto index = pstore::index::get_write_index (*db_);
        ASSERT_NE (index, nullptr);
        auto const actual_values =
            addresses_to_values (*index, std::begin (actual), std::end (actual));
        EXPECT_THAT (actual_values, ::testing::UnorderedElementsAre (v2));
    }

    {
        // Check the diff between r2 and r2.
        pstore::diff::result_type actual =
            pstore::diff::diff (*pstore::index::get_write_index (*db_), 2U);
        EXPECT_EQ (actual.size (), 0U);
    }
}

TEST_F (Diff, UncomittedTransaction) {
    using ::testing::UnorderedElementsAre;

    using value_type = std::pair<std::string, pstore::extent>;
    value_type v1;

    {
        std::string const k1 = "key1";
        transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
        v1 = std::make_pair (k1, this->add (t1, k1, "first value"));
        t1.commit ();
    }

    // The transaction t2 is left uncommitted whilst we perform the diff.
    std::string const k2 = "key2";
    transaction_type t2 = pstore::begin (*db_, lock_guard{mutex_});
    value_type v2 = std::make_pair (k2, this->add (t2, k2, "second value"));

    {
        // Check the diff between now (the uncommitted r2) and r0.
        pstore::diff::result_type actual =
            pstore::diff::diff (*pstore::index::get_write_index (*db_), 0U);
        auto index = pstore::index::get_write_index (*db_);
        ASSERT_NE (index, nullptr);
        auto const actual_values =
            addresses_to_values (*index, std::begin (actual), std::end (actual));
        EXPECT_THAT (actual_values, UnorderedElementsAre (v1, v2));
    }

    {
        // Check the diff between now and r1.
        pstore::diff::result_type actual =
            pstore::diff::diff (*pstore::index::get_write_index (*db_), 1U);
        auto index = pstore::index::get_write_index (*db_);
        ASSERT_NE (index, nullptr);
        auto const actual_values =
            addresses_to_values (*index, std::begin (actual), std::end (actual));
        EXPECT_THAT (actual_values, UnorderedElementsAre (v2));
    }

    {
        // Note that get_current_revision() still reports 1 even though a transaction is open.
        EXPECT_EQ (db_->get_current_revision (), 1U);
        pstore::diff::result_type actual =
            pstore::diff::diff (*pstore::index::get_write_index (*db_), 2U);
        EXPECT_EQ (actual.size (), 0U);
    }

    t2.commit ();
}
// eof: unittests/diff/test_diff.cpp
