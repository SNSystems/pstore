//*  _           _ _                *
//* (_)_ __   __| (_) ___ ___  ___  *
//* | | '_ \ / _` | |/ __/ _ \/ __| *
//* | | | | | (_| | | (_|  __/\__ \ *
//* |_|_| |_|\__,_|_|\___\___||___/ *
//*                                 *
//===- unittests/diff/test_indices.cpp ------------------------------------===//
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

#include "diff/diff_value.hpp"

#include <functional>
#include <memory>
#include <mutex>

// 3rd party includes
#include "gmock/gmock.h"

// pstore
#include "pstore/hamt_map.hpp"
#include "pstore/hamt_set.hpp"
#include "pstore/index_types.hpp"
#include "pstore/sstring_view_archive.hpp"
#include "pstore/transaction.hpp"
#include "pstore_support/sstring_view.hpp"

// local includes
#include "empty_store.hpp"
#include "split.hpp"

namespace {
    class mock_mutex {
    public:
        void lock () {}
        void unlock () {}
    };

    class DiffFixture : public EmptyStore {
    public:
        void SetUp () override;
        void TearDown () override;

        using lock_guard = std::unique_lock<mock_mutex>;
        using transaction_type = pstore::transaction<lock_guard>;

        pstore::extent add (transaction_type & transaction, std::string const & key,
                            std::string const & value);

        void add (transaction_type & transaction, std::string const & key);

        static pstore::sstring_view<std::shared_ptr<char const>>
        make_sstring_view (std::string const & name) {
            auto const length = name.length ();
            auto ptr = std::shared_ptr<char> (new char[length], [](char * p) { delete[] p; });
            std::copy (std::begin (name), std::end (name), ptr.get ());
            return pstore::make_sstring_view (std::static_pointer_cast<char const> (ptr), length);
        }

    protected:
        mock_mutex mutex_;
        std::unique_ptr<pstore::database> db_;
    };

    // SetUp
    // ~~~~~
    void DiffFixture::SetUp () {
        EmptyStore::SetUp ();
        db_.reset (new pstore::database (file_));
        db_->set_vacuum_mode (pstore::database::vacuum_mode::disabled);
    }

    // TearDown
    // ~~~~~~~~
    void DiffFixture::TearDown () {
        db_.reset ();
        EmptyStore::TearDown ();
    }

    // add
    // ~~~
    void DiffFixture::add (transaction_type & transaction, std::string const & key) {
        auto index = pstore::index::get_name_index (*db_);
        index->insert (transaction, pstore::make_sstring_view (key.data (), key.length ()));
    }

    pstore::extent DiffFixture::add (transaction_type & transaction, std::string const & key,
                                     std::string const & value) {

        auto where = pstore::address::null ();
        {
            // Allocate storage for string 'value' and copy the data into it.
            std::shared_ptr<char> ptr;
            std::tie (ptr, where) = transaction.alloc_rw<char> (value.length ());
            std::copy (std::begin (value), std::end (value), ptr.get ());
        }

        auto index = pstore::index::get_write_index (*db_);
        auto const value_extent = pstore::extent{where, value.length ()};
        index->insert_or_assign (transaction, key, value_extent);
        return value_extent;
    }

} // namespace


TEST_F (DiffFixture, BuildNameIndexValues) {
    {
        transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
        this->add (t1, "key1");
        t1.commit ();
    }
    {
        transaction_type t2 = pstore::begin (*db_, lock_guard{mutex_});
        this->add (t2, "key2");
        t2.commit ();
    }
    ASSERT_EQ (2U, db_->get_current_revision ());

    std::set<pstore::sstring_view<std::shared_ptr<char const>>> expected{
        this->make_sstring_view ("key1")};

    auto actual = pstore::diff::build_index_values<pstore::index::name_index> (
        *db_, 1 /*revision */, pstore::index::get_name_index);
    EXPECT_THAT (actual, ::testing::ContainerEq (expected));


    expected.emplace (this->make_sstring_view ("key2"));
    actual = pstore::diff::build_index_values<pstore::index::name_index> (
        *db_, 2 /*revision */, pstore::index::get_name_index);
    EXPECT_THAT (actual, ::testing::ContainerEq (expected));
}


TEST_F (DiffFixture, BuildWriteIndexValues) {
    std::set<std::pair<std::string const, pstore::extent>> expected;
    {
        std::string const k1 = "key1";
        transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
        auto value = this->add (t1, k1, "first value");
        expected.emplace (std::make_pair (k1, value));
        t1.commit ();
    }
    {
        std::string const k2 = "key2";
        transaction_type t2 = pstore::begin (*db_, lock_guard{mutex_});
        auto value = this->add (t2, k2, "second value");
        expected.emplace (std::make_pair (k2, value));
        t2.commit ();
    }
    ASSERT_EQ (2U, db_->get_current_revision ());

    auto actual = pstore::diff::build_index_values<pstore::index::write_index> (
        *db_, 2 /*revision */, pstore::index::get_write_index);
    EXPECT_EQ (actual.size (), expected.size ());
    EXPECT_THAT (actual, ::testing::ContainerEq (expected));
}


TEST_F (DiffFixture, MakeIndexDiffNew2Old1) {
    using ::testing::ElementsAre;

    {
        transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
        this->add (t1, "key1");
        this->add (t1, "key1", "first value");
        t1.commit ();
    }
    {
        transaction_type t2 = pstore::begin (*db_, lock_guard{mutex_});
        this->add (t2, "key2");
        this->add (t2, "key2", "first value");
        t2.commit ();
    }

    auto check = [](std::ostringstream & out, char const * name) {
        auto const lines = split_lines (out.str ());
        ASSERT_EQ (3U, lines.size ());

        auto line = 0U;
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("name", ":", name));
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("members", ":"));
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("-", "key2"));
    };

    std::ostringstream out;
    constexpr auto new_revision = 2U;
    constexpr auto old_revision = 1U;

    pstore::dump::value_ptr addr = pstore::diff::make_index_diff<pstore::index::name_index> (
        "names", *db_, new_revision, old_revision, pstore::index::get_name_index);
    addr->write (out);
    check (out, "names");

    out.clear ();
    out.str ("");
    addr = pstore::diff::make_index_diff<pstore::index::write_index> (
        "write", *db_, new_revision, old_revision, pstore::index::get_write_index);
    addr->write (out);
    check (out, "write");
}

TEST_F (DiffFixture, MakeIndexDiffNew2Old0) {
    using ::testing::ElementsAre;

    {
        transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
        this->add (t1, "key1");
        this->add (t1, "key1", "first value");
        t1.commit ();
    }
    {
        transaction_type t2 = pstore::begin (*db_, lock_guard{mutex_});
        this->add (t2, "key2");
        this->add (t2, "key2", "first value");
        t2.commit ();
    }

    auto check = [](std::ostringstream & out, char const * name) {
        auto const lines = split_lines (out.str ());
        ASSERT_EQ (4U, lines.size ());

        auto line = 0U;
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("name", ":", name));
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("members", ":"));
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("-", "key1"));
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("-", "key2"));
    };

    std::ostringstream out;
    constexpr auto new_revision = 2U;
    constexpr auto old_revision = 0U;

    pstore::dump::value_ptr addr = pstore::diff::make_index_diff<pstore::index::name_index> (
        "names", *db_, new_revision, old_revision, pstore::index::get_name_index);
    addr->write (out);
    check (out, "names");

    out.clear ();
    out.str ("");
    addr = pstore::diff::make_index_diff<pstore::index::write_index> (
        "write", *db_, new_revision, old_revision, pstore::index::get_write_index);
    addr->write (out);
    check (out, "write");
}

TEST_F (DiffFixture, MakeIndexDiffNew1Old1) {
    using ::testing::ElementsAre;

    {
        transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});
        this->add (t1, "key1");
        this->add (t1, "key1", "first value");
        t1.commit ();
    }

    auto check = [](std::ostringstream & out, char const * name) {
        auto const lines = split_lines (out.str ());
        ASSERT_EQ (2U, lines.size ());

        auto line = 0U;
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("name", ":", name));
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("members", ":", "[", "]"));
    };

    std::ostringstream out;
    constexpr auto new_revision = 1U;
    constexpr auto old_revision = 1U;

    pstore::dump::value_ptr addr = pstore::diff::make_index_diff<pstore::index::name_index> (
        "names", *db_, new_revision, old_revision, pstore::index::get_name_index);
    addr->write (out);
    check (out, "names");

    out.clear ();
    out.str ("");
    addr = pstore::diff::make_index_diff<pstore::index::write_index> (
        "write", *db_, new_revision, old_revision, pstore::index::get_write_index);
    addr->write (out);
    check (out, "write");

    out.clear ();
    out.str ("");
    addr = pstore::diff::make_index_diff<pstore::index::name_index> (
        "names", *db_, new_revision, old_revision, pstore::index::get_name_index);
    addr->write (out);
    check (out, "names");

    out.clear ();
    out.str ("");
    addr = pstore::diff::make_index_diff<pstore::index::write_index> (
        "write", *db_, new_revision, old_revision, pstore::index::get_write_index);
    addr->write (out);
    check (out, "write");
}


// eof: unittests/diff/test_indices.cpp
