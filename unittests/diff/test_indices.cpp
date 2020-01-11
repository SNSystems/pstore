//*  _           _ _                *
//* (_)_ __   __| (_) ___ ___  ___  *
//* | | '_ \ / _` | |/ __/ _ \/ __| *
//* | | | | | (_| | | (_|  __/\__ \ *
//* |_|_| |_|\__,_|_|\___\___||___/ *
//*                                 *
//===- unittests/diff/test_indices.cpp ------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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

#include "pstore/diff/diff_value.hpp"

#include <functional>
#include <memory>
#include <mutex>

#include "gmock/gmock.h"

#include "pstore/core/hamt_map.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/indirect_string.hpp"
#include "pstore/core/sstring_view_archive.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/support/sstring_view.hpp"

#include "empty_store.hpp"
#include "mock_mutex.hpp"
#include "split.hpp"

namespace {

    class DiffFixture : public EmptyStore {
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
    void DiffFixture::SetUp () {
        EmptyStore::SetUp ();
        db_.reset (new pstore::database (this->file ()));
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
    pstore::extent<char> DiffFixture::add (transaction_type & transaction, std::string const & key,
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


TEST_F (DiffFixture, MakeIndexDiffNew2Old1) {
    using ::testing::ElementsAre;

    {
        transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});

        pstore::indirect_string_adder adder1;
        auto str1 = pstore::make_sstring_view ("key1");
        adder1.add (t1, pstore::index::get_index<pstore::trailer::indices::name> (*db_), &str1);
        adder1.flush (t1);

        this->add (t1, "key1", "first value");
        t1.commit ();
    }
    {
        transaction_type t2 = pstore::begin (*db_, lock_guard{mutex_});

        pstore::indirect_string_adder adder2;
        auto str2 = pstore::make_sstring_view ("key2");
        adder2.add (t2, pstore::index::get_index<pstore::trailer::indices::name> (*db_), &str2);
        adder2.flush (t2);

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
        "names", *db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::name>);
    addr->write (out);
    check (out, "names");

    out.clear ();
    out.str ("");
    addr = pstore::diff::make_index_diff<pstore::index::write_index> (
        "write", *db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::write>);
    addr->write (out);
    check (out, "write");
}

TEST_F (DiffFixture, MakeIndexDiffNew2Old0) {
    using ::testing::Each;
    using ::testing::ElementsAre;
    using ::testing::UnorderedElementsAre;

    {
        transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});

        pstore::indirect_string_adder adder1;
        auto str1 = pstore::make_sstring_view ("key1");
        adder1.add (t1, pstore::index::get_index<pstore::trailer::indices::name> (*db_), &str1);
        adder1.flush (t1);

        this->add (t1, "key1", "first value");
        t1.commit ();
    }
    {
        transaction_type t2 = pstore::begin (*db_, lock_guard{mutex_});

        pstore::indirect_string_adder adder2;
        auto str2 = pstore::make_sstring_view ("key2");
        adder2.add (t2, pstore::index::get_index<pstore::trailer::indices::name> (*db_), &str2);
        adder2.flush (t2);

        this->add (t2, "key2", "first value");
        t2.commit ();
    }

    auto check = [](std::ostringstream & out, char const * name) {
        auto const lines = split_lines (out.str ());
        ASSERT_EQ (4U, lines.size ());

        auto line = 0U;
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("name", ":", name));
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("members", ":"));

        auto line3 = split_tokens (lines.at (line++));
        ASSERT_EQ (line3.size (), 2U);
        auto line4 = split_tokens (lines.at (line++));
        ASSERT_EQ (line4.size (), 2U);

        std::array <std::string, 2> const actual_prefix{{line3[0], line4[0]}};
        EXPECT_THAT (actual_prefix, Each ("-"));
        std::array<std::string, 2> const actual_keys{{line3[1], line4[1]}};
        EXPECT_THAT (actual_keys, UnorderedElementsAre ("key1", "key2"));
    };

    std::ostringstream out;
    constexpr auto new_revision = 2U;
    constexpr auto old_revision = 0U;

    pstore::dump::value_ptr addr = pstore::diff::make_index_diff<pstore::index::name_index> (
        "names", *db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::name>);
    addr->write (out);
    check (out, "names");

    out.clear ();
    out.str ("");
    addr = pstore::diff::make_index_diff<pstore::index::write_index> (
        "write", *db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::write>);
    addr->write (out);
    check (out, "write");
}

TEST_F (DiffFixture, MakeIndexDiffNew1Old1) {
    using ::testing::ElementsAre;

    {
        transaction_type t1 = pstore::begin (*db_, lock_guard{mutex_});

        pstore::indirect_string_adder adder1;
        auto str1 = pstore::make_sstring_view ("key1");
        adder1.add (t1, pstore::index::get_index<pstore::trailer::indices::name> (*db_), &str1);
        adder1.flush (t1);

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
        "names", *db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::name>);
    addr->write (out);
    check (out, "names");

    out.clear ();
    out.str ("");
    addr = pstore::diff::make_index_diff<pstore::index::write_index> (
        "write", *db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::write>);
    addr->write (out);
    check (out, "write");

    out.clear ();
    out.str ("");
    addr = pstore::diff::make_index_diff<pstore::index::name_index> (
        "names", *db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::name>);
    addr->write (out);
    check (out, "names");

    out.clear ();
    out.str ("");
    addr = pstore::diff::make_index_diff<pstore::index::write_index> (
        "write", *db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::write>);
    addr->write (out);
    check (out, "write");
}
