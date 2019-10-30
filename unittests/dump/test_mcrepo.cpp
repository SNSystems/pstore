//*                                       *
//*  _ __ ___   ___ _ __ ___ _ __   ___   *
//* | '_ ` _ \ / __| '__/ _ \ '_ \ / _ \  *
//* | | | | | | (__| | |  __/ |_) | (_) | *
//* |_| |_| |_|\___|_|  \___| .__/ \___/  *
//*                         |_|           *
//===- unittests/dump/test_mcrepo.cpp -------------------------------------===//
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

#include "pstore/dump/mcrepo_value.hpp"

#include <memory>
#include <vector>

#include "gmock/gmock.h"

#include "pstore/core/db_archive.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/dump/index_value.hpp"
#include "pstore/serialize/standard_types.hpp"
#include "pstore/support/pointee_adaptor.hpp"

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

        pstore::extent<std::uint8_t> store_data (transaction_type & transaction,
                                                 std::uint8_t const * first, std::size_t size);

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
            : buffer_ (pstore::aligned_valloc (file_size_, page_size_))
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
            adder
                .add (transaction, pstore::index::get_index<pstore::trailer::indices::name> (*db_),
                      &sstring)
                .first;
        adder.flush (transaction);
        return pstore::typed_address<pstore::indirect_string> (pos.get_address ());
    }

    // store_data
    // ~~~~~~~~~~
    pstore::extent<std::uint8_t> MCRepoFixture::store_data (transaction_type & transaction,
                                                            std::uint8_t const * first,
                                                            std::size_t size) {

        assert (db_.get () == &transaction.db ());
        // Allocate space in the transaction for 'Size' bytes.
        auto addr = pstore::typed_address<std::uint8_t>::null ();
        std::shared_ptr<std::uint8_t> ptr;
        std::tie (ptr, addr) = transaction.alloc_rw<std::uint8_t> (size);
        std::copy (first, first + size, ptr.get ());
        return {addr, size};
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

    // Build the compilation member 'foo'
    auto const addr =
        pstore::typed_address<compilation_member>{transaction.allocate<compilation_member> ()};
    {
        auto ptr = transaction.getrw (make_extent (addr, sizeof (compilation_member)));
        (*ptr) =
            compilation_member{pstore::index::digest{28U},
                               pstore::extent<pstore::repo::fragment> (
                                   pstore::typed_address<pstore::repo::fragment>::make (5), 7U),
                               name, linkage::internal, visibility::default_vis};
    }

    std::array<pstore::typed_address<compilation_member>, 1> dependents{{addr}};

    // Build the creation dispatchers. These tell fragment::alloc how to build the fragment's various sections.
    std::vector<std::unique_ptr<section_creation_dispatcher>> dispatchers;
    dispatchers.emplace_back (new generic_section_creation_dispatcher (data.kind, &data));
    dispatchers.emplace_back (new dependents_creation_dispatcher (
        dependents.data (), dependents.data () + dependents.size ()));

    auto fragment = fragment::load (
        *db_, fragment::alloc (transaction, pstore::make_pointee_adaptor (dispatchers.begin ()),
                               pstore::make_pointee_adaptor (dispatchers.end ())));

    std::ostringstream out;
    pstore::dump::value_ptr value = pstore::dump::make_fragment_value (
        *db_, *fragment, "machine-vendor-os", false /*hex mode?*/);
    value->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (20U, lines.size ());

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
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("fext", ":", "{", "addr:", "0x5,", "size:", "0x7", "}"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("name", ":", "foo"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("linkage", ":", "internal"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("visibility", ":", "default"));
}

TEST_F (MCRepoFixture, DumpCompilation) {
    using ::testing::ElementsAre;

    // Write output file path "/home/user/test.c"
    transaction_type transaction = pstore::begin (*db_, lock_guard{mutex_});

    std::vector<compilation_member> v{
        {pstore::index::digest{28U},
         pstore::extent<pstore::repo::fragment> (
             pstore::typed_address<pstore::repo::fragment>::make (5), 7U),
         this->store_str (transaction, "main"), linkage::external, visibility::hidden_vis}};
    auto compilation = compilation::load (
        *db_, compilation::alloc (transaction, this->store_str (transaction, "/home/user/"),
                                  this->store_str (transaction, "machine-vendor-os"),
                                  std::begin (v), std::end (v)));

    std::ostringstream out;
    pstore::dump::value_ptr addr = pstore::dump::make_value (*db_, compilation);
    addr->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (8U, lines.size ());

    auto line = 0U;
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("members", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("-", "digest", ":", "0000000000000000000000000000001c"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("fext", ":", "{", "addr:", "0x5,", "size:", "0x7", "}"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("name", ":", "main"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("linkage", ":", "external"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("visibility", ":", "hidden"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("path", ":", "/home/user/"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("triple", ":", "machine-vendor-os"));
}

TEST_F (MCRepoFixture, DumpDebugLineHeader) {
    using ::testing::ElementsAre;
    transaction_type transaction = pstore::begin (*db_, lock_guard{mutex_});

    // debug line header content.
    static std::uint8_t const data[] = {0x44, 0x00, 0x00, 0x00};

    auto debug_line_header_extent = this->store_data (transaction, &data[0], sizeof (data));

    auto index = pstore::index::get_index<pstore::trailer::indices::debug_line_header> (*db_);
    index->insert_or_assign (transaction, pstore::index::digest{1U}, debug_line_header_extent);
    transaction.commit ();

    std::ostringstream out;
    pstore::dump::value_ptr addr =
        pstore::dump::make_index<pstore::trailer::indices::debug_line_header> (
            *db_, [this](pstore::index::debug_line_header_index::value_type const & value) {
                return pstore::dump::make_value (*this->db_, value, true);
            });

    addr->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (4U, lines.size ());

    auto line = 0U;
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ());
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("-", "digest", ":", "00000000000000000000000000000001"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("debug_line_header", ":", "!!binary16", "|"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("4400", "0000>"));
}

TEST_F (MCRepoFixture, DumpBssSection) {
    using ::testing::ElementsAre;
    using ::testing::ElementsAreArray;

    // Create a fragment which containing a BSS section.
    std::shared_ptr<fragment const> frag = [this] () {
        transaction_type transaction = pstore::begin (*db_, lock_guard{mutex_});

        std::array<section_content, 1> c = {
            {section_content (section_kind::bss, std::uint8_t{0x10} /*alignment*/)}};

        section_content & data = c.back ();
        // Build the bss section's data, no internal and external fixups. (Note that this is used by the dispatcher and it doesn't end up in the resulting bss_section instance.)
        data.data.assign ({0, 0, 0, 0});

        // Build the creation dispatchers. These tell fragment::alloc how to build the fragment's
        // various sections.
        std::vector<std::unique_ptr<section_creation_dispatcher>> dispatchers;
        dispatchers.emplace_back (new bss_section_creation_dispatcher (&data));

        std::shared_ptr<fragment const> f = fragment::load (
            *db_, fragment::alloc (transaction, pstore::make_pointee_adaptor (dispatchers.begin ()),
                                   pstore::make_pointee_adaptor (dispatchers.end ())));
        transaction.commit ();
        return f;
    } ();

    std::ostringstream out;
    pstore::dump::value_ptr value = pstore::dump::make_fragment_value (
        *db_, *frag, "machine-vendor-os", false /*hex mode?*/);
    value->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (5U, lines.size ());

    auto line = 0U;
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ());
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("-", "type", ":", "bss"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("contents", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("align", ":", "0x10"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("size", ":", "0x4"));
}
