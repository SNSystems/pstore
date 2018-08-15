//*                                       *
//*  _ __ ___   ___ _ __ ___ _ __   ___   *
//* | '_ ` _ \ / __| '__/ _ \ '_ \ / _ \  *
//* | | | | | | (__| | |  __/ |_) | (_) | *
//* |_| |_| |_|\___|_|  \___| .__/ \___/  *
//*                         |_|           *
//===- unittests/dump/test_mcrepo.cpp -------------------------------------===//
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

#include "pstore/dump/mcrepo_value.hpp"

#include <memory>
#include <vector>

#include "gmock/gmock.h"

#include "pstore/core/db_archive.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/serialize/standard_types.hpp"

#include "empty_store.hpp"
#include "mock_mutex.hpp"
#include "split.hpp"

using namespace pstore::repo;

//*  __  __  ___ ___               ___ _     _                 *
//* |  \/  |/ __| _ \___ _ __  ___| __(_)_ _| |_ _  _ _ _ ___  *
//* | |\/| | (__|   / -_) '_ \/ _ \ _|| \ \ /  _| || | '_/ -_) *
//* |_|  |_|\___|_|_\___| .__/\___/_| |_/_\_\\__|\_,_|_| \___| *
//*                     |_|                                    *
namespace {

    class MCRepoFixture : public ::testing::Test {
    public:
        MCRepoFixture ();

        using lock_guard = std::unique_lock<mock_mutex>;
        using transaction_type = pstore::transaction<lock_guard>;

    protected:
        pstore::typed_address<pstore::indirect_string> store_str (transaction_type & transaction,
                                                                  std::string const & str);

        static constexpr std::size_t page_size_ = 4096;
        static constexpr std::size_t file_size_ = pstore::storage::min_region_size * 2;

        mock_mutex mutex_;
        std::shared_ptr<std::uint8_t> buffer_;
        std::shared_ptr<pstore::file::in_memory> file_;
        std::unique_ptr<pstore::database> db_;
    };

    constexpr std::size_t MCRepoFixture::page_size_;
    constexpr std::size_t MCRepoFixture::file_size_;

    // ctor
    // ~~~~
    MCRepoFixture::MCRepoFixture ()
            : buffer_ (aligned_valloc (file_size_, page_size_))
            , file_ (std::make_shared<pstore::file::in_memory> (buffer_, file_size_)) {
        pstore::database::build_new_store (*file_);
        db_.reset (new pstore::database (file_));
    }

    // store_str
    // ~~~~~~~~~
    pstore::typed_address<pstore::indirect_string>
    MCRepoFixture::store_str (transaction_type & transaction, std::string const & str) {
        assert (db_.get () == &transaction.db ());
        pstore::raw_sstring_view const sstring = pstore::make_sstring_view (str);
        pstore::indirect_string_adder adder;
        auto const pos =
            adder.add (transaction, pstore::index::get_name_index (*db_), &sstring).first;
        adder.flush (transaction);
        return pstore::typed_address<pstore::indirect_string> (pos.get_address ());
    }

} // end anonymous namespace

TEST_F (MCRepoFixture, DumpFragment) {
    using ::testing::ElementsAre;
    using ::testing::ElementsAreArray;

    transaction_type transaction = pstore::begin (*db_, lock_guard{mutex_});

    std::array<section_content, 1> c = {
        {section_content (section_kind::data, std::uint8_t{0x10} /*alignment*/)}};
    section_content & data = c.back ();
    pstore::typed_address<pstore::indirect_string> name = this->store_str (transaction, "foo");
    {
        // Build the data section's contents and fixups.
        data.data.assign ({'t', 'e', 'x', 't'});
        data.ifixups.emplace_back (internal_fixup{section_kind::data, 2, 2, 2});
        data.xfixups.emplace_back (external_fixup{name, 3, 3, 3});
    }

    // Build the ticket_member 'foo'
    auto const addr = pstore::typed_address<ticket_member>{transaction.allocate<ticket_member> ()};
    {
        auto ptr = transaction.getrw (make_extent (addr, sizeof (ticket_member)));
        (*ptr) = ticket_member{pstore::index::digest{28U}, name, linkage_type::internal};
    }

    std::array<pstore::typed_address<ticket_member>, 1> dependents{{addr}};

    // Build the creation dispatchers. These tell fragment::alloc how to build the fragment's various sections.
    std::vector<std::unique_ptr<section_creation_dispatcher>> dispatchers;
    dispatchers.emplace_back (new generic_section_creation_dispatcher (data.kind, &data));
    dispatchers.emplace_back (
        new dependents_creation_dispatcher (dependents.data (), dependents.data () + dependents.size ()));

    auto fragment = fragment::load (
        *db_, fragment::alloc (transaction,
                               details::make_fragment_content_iterator (dispatchers.begin ()),
                               details::make_fragment_content_iterator (dispatchers.end ())));

    std::ostringstream out;
    pstore::dump::value_ptr value = pstore::dump::make_value (*db_, *fragment, false /*hex mode?*/);
    value->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (18U, lines.size ());

    auto line = 0U;
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ());
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("-", "type", ":", "data"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("contents", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("align", ":", "0x10"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("data", ":", "!!binary", "|"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("dGV4dA=="));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("ifixups", ":"));

    char const * expected_ifixup[] = {"-",       "{",    "section:", "data,", "type:", "0x2,",
                                      "offset:", "0x2,", "addend:",  "0x2",   "}"};
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAreArray (expected_ifixup));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("xfixups", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("-", "name", ":", "foo"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("type", ":", "0x3"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("offset", ":", "0x3"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("addend", ":", "0x3"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("-", "type", ":", "dependent"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("contents", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("-", "digest", ":", "0000000000000000000000000000001c"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("name", ":", "foo"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("linkage", ":", "internal"));
}

TEST_F (MCRepoFixture, DumpTicket) {
    using ::testing::ElementsAre;

    // Write output file path "/home/user/test.c"
    transaction_type transaction = pstore::begin (*db_, lock_guard{mutex_});

    // ticket_member sm{digest, name, linkage};
    std::vector<ticket_member> v{{pstore::index::digest{28U}, this->store_str (transaction, "main"),
                                  linkage_type::external}};
    auto ticket = ticket::load (
        *db_, ticket::alloc (transaction, this->store_str (transaction, "/home/user/"), v));

    std::ostringstream out;
    pstore::dump::value_ptr addr = pstore::dump::make_value (*db_, ticket);
    addr->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (5U, lines.size ());

    auto line = 0U;
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("members", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("-", "digest", ":", "0000000000000000000000000000001c"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("name", ":", "main"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("linkage", ":", "external"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("path", ":", "/home/user/"));
}
