//===- unittests/diff/test_indices.cpp ------------------------------------===//
//*  _           _ _                *
//* (_)_ __   __| (_) ___ ___  ___  *
//* | | '_ \ / _` | |/ __/ _ \/ __| *
//* | | | | | (_| | | (_|  __/\__ \ *
//* |_|_| |_|\__,_|_|\___\___||___/ *
//*                                 *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/diff_dump/diff_value.hpp"

// Standard library includes
#include <functional>
#include <memory>
#include <mutex>

// 3rd party includes
#include <gmock/gmock.h>

// pstore includes
#include "pstore/adt/sstring_view.hpp"
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/indirect_string.hpp"
#include "pstore/core/sstring_view_archive.hpp"
#include "pstore/core/transaction.hpp"

// Local includes
#include "empty_store.hpp"
#include "split.hpp"

namespace {

    class DiffFixture : public testing::Test {
    public:
        DiffFixture ();

        using lock_guard = std::unique_lock<mock_mutex>;
        using transaction_type = pstore::transaction<lock_guard>;

        pstore::extent<char> add (transaction_type & transaction, std::string const & key,
                                  std::string const & value);

    protected:
        mock_mutex mutex_;
        in_memory_store store_;
        pstore::database db_;
    };

    // (ctor)
    // ~~~~~~
    DiffFixture::DiffFixture ()
            : db_{store_.file ()} {
        db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
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

        auto index = pstore::index::get_index<pstore::trailer::indices::write> (db_);
        auto const value_extent = make_extent (where, value.length ());
        index->insert_or_assign (transaction, key, value_extent);
        return value_extent;
    }

} // end anonymous namespace

TEST_F (DiffFixture, MakeIndexDiffNew2Old1) {
    using ::testing::ElementsAre;

    {
        transaction_type t1 = begin (db_, lock_guard{mutex_});

        pstore::indirect_string_adder adder1;
        auto str1 = pstore::make_sstring_view ("key1");
        adder1.add (t1, pstore::index::get_index<pstore::trailer::indices::name> (db_), &str1);
        adder1.flush (t1);

        this->add (t1, "key1", "first value");
        t1.commit ();
    }
    {
        transaction_type t2 = begin (db_, lock_guard{mutex_});

        pstore::indirect_string_adder adder2;
        auto str2 = pstore::make_sstring_view ("key2");
        adder2.add (t2, pstore::index::get_index<pstore::trailer::indices::name> (db_), &str2);
        adder2.flush (t2);

        this->add (t2, "key2", "first value");
        t2.commit ();
    }

    auto check = [] (std::ostringstream & out, char const * name) {
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

    pstore::dump::value_ptr addr = pstore::diff_dump::make_index_diff<pstore::index::name_index> (
        "names", db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::name>);
    addr->write (out);
    check (out, "names");

    out.clear ();
    out.str ("");
    addr = pstore::diff_dump::make_index_diff<pstore::index::write_index> (
        "write", db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::write>);
    addr->write (out);
    check (out, "write");
}

TEST_F (DiffFixture, MakeIndexDiffNew2Old0) {
    using ::testing::Each;
    using ::testing::ElementsAre;
    using ::testing::UnorderedElementsAre;

    {
        transaction_type t1 = begin (db_, lock_guard{mutex_});

        pstore::indirect_string_adder adder1;
        auto str1 = pstore::make_sstring_view ("key1");
        adder1.add (t1, pstore::index::get_index<pstore::trailer::indices::name> (db_), &str1);
        adder1.flush (t1);

        this->add (t1, "key1", "first value");
        t1.commit ();
    }
    {
        transaction_type t2 = begin (db_, lock_guard{mutex_});

        pstore::indirect_string_adder adder2;
        auto str2 = pstore::make_sstring_view ("key2");
        adder2.add (t2, pstore::index::get_index<pstore::trailer::indices::name> (db_), &str2);
        adder2.flush (t2);

        this->add (t2, "key2", "first value");
        t2.commit ();
    }

    auto check = [] (std::ostringstream & out, char const * name) {
        auto const lines = split_lines (out.str ());
        ASSERT_EQ (4U, lines.size ());

        auto line = 0U;
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("name", ":", name));
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("members", ":"));

        auto line3 = split_tokens (lines.at (line++));
        ASSERT_EQ (line3.size (), 2U);
        auto line4 = split_tokens (lines.at (line++));
        ASSERT_EQ (line4.size (), 2U);

        std::array<std::string, 2> const actual_prefix{{line3[0], line4[0]}};
        EXPECT_THAT (actual_prefix, Each ("-"));
        std::array<std::string, 2> const actual_keys{{line3[1], line4[1]}};
        EXPECT_THAT (actual_keys, UnorderedElementsAre ("key1", "key2"));
    };

    std::ostringstream out;
    constexpr auto new_revision = 2U;
    constexpr auto old_revision = 0U;

    pstore::dump::value_ptr addr = pstore::diff_dump::make_index_diff<pstore::index::name_index> (
        "names", db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::name>);
    addr->write (out);
    check (out, "names");

    out.clear ();
    out.str ("");
    addr = pstore::diff_dump::make_index_diff<pstore::index::write_index> (
        "write", db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::write>);
    addr->write (out);
    check (out, "write");
}

TEST_F (DiffFixture, MakeIndexDiffNew1Old1) {
    using ::testing::ElementsAre;

    {
        transaction_type t1 = begin (db_, lock_guard{mutex_});

        pstore::indirect_string_adder adder1;
        auto str1 = pstore::make_sstring_view ("key1");
        adder1.add (t1, pstore::index::get_index<pstore::trailer::indices::name> (db_), &str1);
        adder1.flush (t1);

        this->add (t1, "key1", "first value");
        t1.commit ();
    }

    auto check = [] (std::ostringstream & out, char const * name) {
        auto const lines = split_lines (out.str ());
        ASSERT_EQ (2U, lines.size ());

        auto line = 0U;
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("name", ":", name));
        EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("members", ":", "[", "]"));
    };

    std::ostringstream out;
    constexpr auto new_revision = 1U;
    constexpr auto old_revision = 1U;

    pstore::dump::value_ptr addr = pstore::diff_dump::make_index_diff<pstore::index::name_index> (
        "names", db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::name>);
    addr->write (out);
    check (out, "names");

    out.clear ();
    out.str ("");
    addr = pstore::diff_dump::make_index_diff<pstore::index::write_index> (
        "write", db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::write>);
    addr->write (out);
    check (out, "write");

    out.clear ();
    out.str ("");
    addr = pstore::diff_dump::make_index_diff<pstore::index::name_index> (
        "names", db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::name>);
    addr->write (out);
    check (out, "names");

    out.clear ();
    out.str ("");
    addr = pstore::diff_dump::make_index_diff<pstore::index::write_index> (
        "write", db_, new_revision, old_revision,
        pstore::index::get_index<pstore::trailer::indices::write>);
    addr->write (out);
    check (out, "write");
}
