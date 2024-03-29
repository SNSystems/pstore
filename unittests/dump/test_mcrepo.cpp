//===- unittests/dump/test_mcrepo.cpp -------------------------------------===//
//*                                       *
//*  _ __ ___   ___ _ __ ___ _ __   ___   *
//* | '_ ` _ \ / __| '__/ _ \ '_ \ / _ \  *
//* | | | | | | (__| | |  __/ |_) | (_) | *
//* |_| |_| |_|\___|_|  \___| .__/ \___/  *
//*                         |_|           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/dump/mcrepo_value.hpp"

// Standard library includes
#include <memory>
#include <vector>

// 3rd party includes
#include <gmock/gmock.h>

// pstore includes
#include "pstore/core/db_archive.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/dump/index_value.hpp"
#include "pstore/serialize/standard_types.hpp"
#include "pstore/support/pointee_adaptor.hpp"

// Local includes
#include "empty_store.hpp"
#include "split.hpp"

using namespace pstore::repo;

TEST (MCRepoSectionContent, DefaultCtor) {
    section_content c;
    EXPECT_EQ (c.kind, section_kind::text);
    EXPECT_EQ (c.align, 1U);
    EXPECT_TRUE (c.data.empty ());
    EXPECT_TRUE (c.ifixups.empty ());
    EXPECT_TRUE (c.xfixups.empty ());
}

TEST (MCRepoSectionContent, Eq) {
    auto const populate = [] (section_content * const sc) {
        sc->kind = section_kind::data;
        sc->align = 8U;
        sc->data.clear ();
        std::fill_n (std::back_inserter (sc->data), 7U, std::uint8_t{7});
        sc->ifixups.clear ();
        std::fill_n (std::back_inserter (sc->ifixups), 3U,
                     internal_fixup (section_kind::read_only, relocation_type{1}, 5U /*offset*/,
                                     13U /*addend*/));
        // TODO: xfixups.
    };

    section_content c1;
    populate (&c1);
    section_content c2;
    populate (&c2);
    EXPECT_TRUE (operator== (c1, c2));
}

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
        PSTORE_ASSERT (db_.get () == &transaction.db ());
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

        PSTORE_ASSERT (db_.get () == &transaction.db ());
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

    transaction_type transaction = begin (*db_, lock_guard{mutex_});

    std::array<section_content, 1> c = {{{section_kind::text, std::uint8_t{0x10} /*alignment*/}}};
    section_content & data = c.back ();
    pstore::typed_address<pstore::indirect_string> name = this->store_str (transaction, "foo");
    {
        // Build the data section's contents and fixups.

        // The contents don't really matter, but this is x86_64 code that would disassemble to:
        //  - 0: 55              pushq   %rbp
        //  - 1: 48 89 E5        movq    %rsp, %rbp
        //  - 4: B8 2B 00 00 00  movl    $43, %eax
        //  - 9: 5D              popq    %rbp
        //  - A: C3              retq
        data.data.assign ({0x55, 0x48, 0x89, 0xE5, 0xB8, 0x2B, 0x00, 0x00, 0x00, 0x5D, 0xC3});
        data.ifixups.emplace_back (internal_fixup{section_kind::data, relocation_type{2},
                                                  UINT64_C (2) /*offset*/, INT64_C (2) /*addend*/});
        data.xfixups.emplace_back (external_fixup{name, relocation_type{3},
                                                  pstore::repo::binding::strong,
                                                  UINT64_C (3) /*offset*/, INT32_C (3) /*addend*/});
    }

    // Build the definition for 'foo'
    auto const addr = pstore::typed_address<definition>{transaction.allocate<definition> ()};
    {
        auto ptr = transaction.getrw (make_extent (addr, sizeof (definition)));
        (*ptr) = definition{pstore::index::digest{28U},
                            pstore::extent<pstore::repo::fragment> (
                                pstore::typed_address<pstore::repo::fragment>::make (5), 7U),
                            name, linkage::internal, visibility::default_vis};
    }

    std::array<linked_definitions::value_type, 1> linked_definitions{
        {linked_definitions::value_type{
            pstore::index::digest{UINT64_C (0x123456789ABCDEF), UINT64_C (0xFEDCBA9876543210)}, 13U,
            addr}}};

    // Build the creation dispatchers. These tell fragment::alloc how to build the fragment's
    // various sections.
    std::vector<std::unique_ptr<section_creation_dispatcher>> dispatchers;
    dispatchers.emplace_back (new generic_section_creation_dispatcher (data.kind, &data));
    dispatchers.emplace_back (new linked_definitions_creation_dispatcher (
        linked_definitions.data (), linked_definitions.data () + linked_definitions.size ()));

    auto fragment = fragment::load (
        *db_, fragment::alloc (transaction, pstore::make_pointee_adaptor (dispatchers.begin ()),
                               pstore::make_pointee_adaptor (dispatchers.end ())));

    constexpr bool hex_mode = false;
    constexpr bool expanded_addresses = false;
    constexpr bool no_times = false;
    constexpr bool no_disassembly = true;
    pstore::dump::parameters parms{*db_,     hex_mode,       expanded_addresses,
                                   no_times, no_disassembly, "x86_64-pc-linux-gnu-repo"};
    std::ostringstream out;
    pstore::dump::value_ptr value = pstore::dump::make_fragment_value (*fragment, parms);
    value->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (24U, lines.size ());

    auto line = 0U;
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ());
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("-", "type", ":", "text"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("contents", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("align", ":", "0x10"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("data", ":", "!!binary", "|"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("VUiJ5bgrAAAAXcM="));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("ifixups", ":"));

    // If building inside LLVM, dump will use the ELF name for the relocation rather than its raw
    // numeric value.
#ifdef PSTORE_IS_INSIDE_LLVM
    constexpr auto reloc2_name = "R_X86_64_PC32";
    constexpr auto reloc3_name = "R_X86_64_GOT32";
#else
    constexpr auto reloc2_name = "0x2";
    constexpr auto reloc3_name = "0x3";
#endif // PSTORE_IS_INSIDE_LLVM

    std::string const expected_ifixup[] = {
        "-",       "{",    "section:", "data,", "type:", std::string{reloc2_name} + ",",
        "offset:", "0x2,", "addend:",  "0x2",   "}"};
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAreArray (expected_ifixup));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("xfixups", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("-", "name", ":", "foo"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("type", ":", reloc3_name));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("is_weak", ":", "false"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("offset", ":", "0x3"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("addend", ":", "0x3"));

    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("-", "type", ":", "linked_definitions"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("contents", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("-", "compilation", ":", "0123456789abcdeffedcba9876543210"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("index", ":", "0xd"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("pointer", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("digest", ":", "0000000000000000000000000000001c"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("fext", ":", "{", "addr:", "0x5,", "size:", "0x7", "}"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("name", ":", "foo"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("linkage", ":", "internal"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("visibility", ":", "default"));
}

TEST_F (MCRepoFixture, DumpCompilation) {
    using ::testing::ElementsAre;

    // Write output file path "/home/user/test.c"
    transaction_type transaction = begin (*db_, lock_guard{mutex_});

    std::vector<definition> v{{pstore::index::digest{28U},
                               pstore::extent<pstore::repo::fragment> (
                                   pstore::typed_address<pstore::repo::fragment>::make (5), 7U),
                               this->store_str (transaction, "main"), linkage::external,
                               visibility::hidden_vis}};
    auto compilation = compilation::load (
        *db_, compilation::alloc (transaction, this->store_str (transaction, "machine-vendor-os"),
                                  std::begin (v), std::end (v)));

    std::ostringstream out;
    constexpr bool hex_mode = false;
    constexpr bool expanded_addresses = false;
    constexpr bool no_times = false;
    constexpr bool no_disassembly = false;
    pstore::dump::parameters parms{*db_,     hex_mode,       expanded_addresses,
                                   no_times, no_disassembly, "machine-vendor-os"};
    pstore::dump::value_ptr addr = pstore::dump::make_value (compilation, parms);
    addr->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (7U, lines.size ());

    auto line = 0U;
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("members", ":"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("-", "digest", ":", "0000000000000000000000000000001c"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("fext", ":", "{", "addr:", "0x5,", "size:", "0x7", "}"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("name", ":", "main"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("linkage", ":", "external"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("visibility", ":", "hidden"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("triple", ":", "machine-vendor-os"));
}

TEST_F (MCRepoFixture, DumpDebugLineHeader) {
    using ::testing::ElementsAre;
    transaction_type transaction = begin (*db_, lock_guard{mutex_});

    // debug line header content.
    static std::uint8_t const data[] = {0x44, 0x00, 0x00, 0x00};

    auto debug_line_header_extent = this->store_data (transaction, &data[0], sizeof (data));

    auto index = pstore::index::get_index<pstore::trailer::indices::debug_line_header> (*db_);
    index->insert_or_assign (transaction, pstore::index::digest{1U}, debug_line_header_extent);
    transaction.commit ();

    std::ostringstream out;
    constexpr bool hex_mode = true;
    constexpr bool expanded_addresses = false;
    constexpr bool no_times = false;
    constexpr bool no_disassembly = false;
    pstore::dump::parameters parms{*db_,     hex_mode,       expanded_addresses,
                                   no_times, no_disassembly, "machine-vendor-os"};
    pstore::dump::value_ptr addr =
        pstore::dump::make_index<pstore::trailer::indices::debug_line_header> (
            *db_, [&parms] (pstore::index::debug_line_header_index::value_type const & value) {
                return pstore::dump::make_value (value, parms);
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
        transaction_type transaction = begin (*db_, lock_guard{mutex_});

        std::array<section_content, 1> c = {
            {{section_kind::bss, std::uint8_t{0x10} /*alignment*/}}};

        section_content & data = c.back ();
        // Build the bss section's data, no internal and external fixups. (Note that this is used by
        // the dispatcher and it doesn't end up in the resulting bss_section instance.)
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
    }();

    std::ostringstream out;
    constexpr bool hex_mode = false;
    constexpr bool expanded_addresses = false;
    constexpr bool no_times = false;
    constexpr bool no_disassembly = false;
    pstore::dump::parameters parms{*db_,     hex_mode,       expanded_addresses,
                                   no_times, no_disassembly, "machine-vendor-os"};
    pstore::dump::value_ptr value = pstore::dump::make_fragment_value (*frag, parms);
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
