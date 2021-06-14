//===- unittests/exchange/test_compilation.cpp ----------------------------===//
//*                            _ _       _   _              *
//*   ___ ___  _ __ ___  _ __ (_) | __ _| |_(_) ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ \| | |/ _` | __| |/ _ \| '_ \  *
//* | (_| (_) | | | | | | |_) | | | (_| | |_| | (_) | | | | *
//*  \___\___/|_| |_| |_| .__/|_|_|\__,_|\__|_|\___/|_| |_| *
//*                     |_|                                 *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/export_compilation.hpp"
#include "pstore/exchange/import_compilation.hpp"

// Standard library
#include <array>
#include <sstream>
#include <unordered_map>

// 3rd party includes
#include <gtest/gtest.h>

// pstore includes
#include "pstore/exchange/export_fragment.hpp"
#include "pstore/exchange/export_strings.hpp"
#include "pstore/exchange/import_fragment.hpp"
#include "pstore/exchange/import_strings_array.hpp"
#include "pstore/json/json.hpp"
#include "pstore/mcrepo/fragment.hpp"

// local includes
#include "add_export_strings.hpp"

using namespace std::literals::string_literals;

namespace {

    using transaction_lock = std::unique_lock<mock_mutex>;
    using transaction = pstore::transaction<transaction_lock>;

    template <typename ImportRule, typename... Args>
    auto make_json_array_parser (pstore::database * const db, Args... args)
        -> pstore::json::parser<pstore::exchange::import_ns::callbacks> {
        using namespace pstore::exchange::import_ns;
        return pstore::json::make_parser (
            callbacks::make<array_rule<ImportRule, Args...>> (db, args...));
    }

    template <typename ImportRule, typename... Args>
    auto make_json_object_parser (pstore::database * const db, Args... args)
        -> pstore::json::parser<pstore::exchange::import_ns::callbacks> {
        using namespace pstore::exchange::import_ns;
        return pstore::json::make_parser (
            callbacks::make<object_rule<ImportRule, Args...>> (db, args...));
    }


    // Parse the exported strings JSON. The resulting index-to-string mappings are then available
    // via imported_names.
    decltype (auto)
    import_strings_parser (transaction * const transaction,
                           pstore::exchange::import_ns::string_mapping * const names) {
        return make_json_array_parser<pstore::exchange::import_ns::strings_array_members> (
            &transaction->db (), transaction, names);
    }

    decltype (auto)
    import_fragment_parser (transaction * const transaction,
                            pstore::exchange::import_ns::string_mapping * const names,
                            pstore::index::digest const * const digest) {
        return make_json_object_parser<pstore::exchange::import_ns::fragment_sections> (
            &transaction->db (), transaction, names, digest);
    }

    decltype (auto) import_compilation_parser (
        transaction * const transaction, pstore::exchange::import_ns::string_mapping * const names,
        std::shared_ptr<pstore::index::fragment_index> const & fragment_index,
        pstore::index::digest const & digest) {
        return make_json_object_parser<pstore::exchange::import_ns::compilation> (
            &transaction->db (), transaction, names, std::cref (fragment_index),
            std::cref (digest));
    }


    class ExchangeCompilation : public testing::Test {
    public:
        ExchangeCompilation ()
                : export_db_{export_store_.file ()}
                , import_db_{import_store_.file ()} {
            export_db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
            import_db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

        InMemoryStore export_store_;
        pstore::database export_db_;

        InMemoryStore import_store_;
        pstore::database import_db_;
    };


    pstore::extent<pstore::repo::fragment> build_fragment (transaction & transaction,
                                                           pstore::index::digest const & digest) {
        pstore::repo::section_content content{pstore::repo::section_kind::bss};
        content.align = 4U;
        content.data.resize (128U);

        std::array<pstore::repo::bss_section_creation_dispatcher, 1> dispatcher;
        dispatcher[0].set_content (&content);
        pstore::extent<pstore::repo::fragment> const fext = pstore::repo::fragment::alloc (
            transaction, std::begin (dispatcher), std::end (dispatcher));

        auto fragment_index =
            pstore::index::get_index<pstore::trailer::indices::fragment> (transaction.db ());
        fragment_index->insert (transaction, std::make_pair (digest, fext));

        return fext;
    }

    std::string load_string (pstore::database const & db,
                             pstore::typed_address<pstore::indirect_string> const addr) {
        return pstore::indirect_string::read (db, addr).to_string ();
    }

} // end anonymous namespace


TEST_F (ExchangeCompilation, Empty) {
    using namespace pstore::exchange;
    static constexpr auto comments = false;
    static constexpr auto name_index = pstore::trailer::indices::name;

    constexpr auto * triple = "triple";
    std::array<pstore::gsl::czstring, 1> names{{triple}};
    std::unordered_map<std::string, pstore::typed_address<pstore::indirect_string>> indir_strings;
    add_export_strings<name_index> (export_db_, std::begin (names), std::end (names),
                                    std::inserter (indir_strings, std::end (indir_strings)));

    // Write the names that we just created as JSON.
    export_ns::string_mapping exported_names{export_db_, export_ns::name_index_tag ()};
    export_ns::ostringstream exported_names_stream;
    export_ns::emit_strings<name_index> (exported_names_stream, export_ns::indent{}, export_db_,
                                         export_db_.get_current_revision (), "", &exported_names,
                                         comments);

    export_ns::ostringstream exported_compilation_stream;
    {
        mock_mutex mutex;
        auto transaction = begin (export_db_, transaction_lock{mutex});
        std::vector<pstore::repo::definition> definitions;
        pstore::extent<pstore::repo::compilation> const compilation =
            pstore::repo::compilation::alloc (transaction, indir_strings[triple],
                                              std::begin (definitions), std::end (definitions));

        emit_compilation (exported_compilation_stream, export_ns::indent{}, export_db_,
                          *export_db_.getro (compilation), exported_names, comments);
        transaction.commit ();
    }



    constexpr pstore::index::digest compilation_digest{0x12345678, 0x9ABCDEF0};
    import_ns::string_mapping imported_names;
    {
        mock_mutex mutex;
        auto transaction = begin (import_db_, transaction_lock{mutex});
        auto name_parser = import_strings_parser (&transaction, &imported_names);
        name_parser.input (exported_names_stream.str ()).eof ();
        ASSERT_FALSE (name_parser.has_error ())
            << "JSON error was: " << name_parser.last_error ().message () << ' '
            << name_parser.coordinate () << '\n'
            << exported_names_stream.str ();

        auto const fragment_index =
            pstore::index::get_index<pstore::trailer::indices::fragment> (import_db_);
        auto compilation_parser = import_compilation_parser (&transaction, &imported_names,
                                                             fragment_index, compilation_digest);
        compilation_parser.input (exported_compilation_stream.str ()).eof ();
        ASSERT_FALSE (compilation_parser.has_error ())
            << "JSON error was: " << compilation_parser.last_error ().message () << ' '
            << compilation_parser.coordinate () << '\n'
            << exported_compilation_stream.str ();

        transaction.commit ();
    }


    auto const compilation_index =
        pstore::index::get_index<pstore::trailer::indices::compilation> (import_db_);
    auto const pos = compilation_index->find (import_db_, compilation_digest);
    ASSERT_NE (pos, compilation_index->end (import_db_))
        << "Compilation was not found in the index after import";
    auto const imported_compilation = import_db_.getro (pos->second);
    EXPECT_EQ (load_string (import_db_, imported_compilation->triple ()), triple);
    ASSERT_EQ (imported_compilation->size (), 0U)
        << "The compilation should contain no definitions";
}

TEST_F (ExchangeCompilation, TwoDefinitions) {
    using namespace pstore::exchange;
    constexpr auto comments = false;

    // Add names to the store so that external fixups can use then. add_export_strings()
    // yields a mapping from each name to its indirect-address.
    constexpr auto * name1 = "name1";
    constexpr auto * name2 = "name2";
    constexpr auto * triple = "triple";
    constexpr auto name_index = pstore::trailer::indices::name;

    std::array<pstore::gsl::czstring, 3> names{{triple, name1, name2}};
    std::unordered_map<std::string, pstore::typed_address<pstore::indirect_string>> indir_strings;
    add_export_strings<name_index> (export_db_, std::begin (names), std::end (names),
                                    std::inserter (indir_strings, std::end (indir_strings)));

    // Write the names that we just created as JSON.
    export_ns::string_mapping exported_names{export_db_, export_ns::name_index_tag ()};
    export_ns::ostringstream exported_names_stream;
    export_ns::emit_strings<name_index> (exported_names_stream, export_ns::indent{}, export_db_,
                                         export_db_.get_current_revision (), "", &exported_names,
                                         comments);

    // Now build a single fragment and a compilation that references it then export them.
    constexpr pstore::index::digest compilation_digest{0x12345678, 0x9ABCDEF0};
    constexpr pstore::index::digest fragment_digest{0x9ABCDEF0, 0x12345678};

    export_ns::ostringstream exported_compilation_stream;
    export_ns::ostringstream exported_fragment_stream;

    {
        mock_mutex mutex;
        auto transaction = begin (export_db_, transaction_lock{mutex});
        pstore::extent<pstore::repo::fragment> const fext =
            build_fragment (transaction, fragment_digest);
        emit_fragment (exported_fragment_stream, export_ns::indent{}, export_db_, exported_names,
                       export_db_.getro (fext), comments);

        std::vector<pstore::repo::definition> definitions{
            {fragment_digest, fext, indir_strings[name1], pstore::repo::linkage::external,
             pstore::repo::visibility::hidden_vis},
            {fragment_digest, fext, indir_strings[name2], pstore::repo::linkage::link_once_any,
             pstore::repo::visibility::default_vis},
        };
        pstore::extent<pstore::repo::compilation> const compilation =
            pstore::repo::compilation::alloc (transaction, indir_strings[triple],
                                              std::begin (definitions), std::end (definitions));

        emit_compilation (exported_compilation_stream, export_ns::indent{}, export_db_,
                          *export_db_.getro (compilation), exported_names, comments);
        transaction.commit ();
    }


    // Now begin to export the three pieces: the names, the fragment, and finally the compilation.
    mock_mutex mutex;
    auto transaction = begin (import_db_, transaction_lock{mutex});


    import_ns::string_mapping imported_names;
    {
        auto name_parser = import_strings_parser (&transaction, &imported_names);
        name_parser.input (exported_names_stream.str ()).eof ();
        ASSERT_FALSE (name_parser.has_error ())
            << "JSON error was: " << name_parser.last_error ().message () << ' '
            << name_parser.coordinate () << '\n'
            << exported_names_stream.str ();
    }
    {
        auto fragment_parser =
            import_fragment_parser (&transaction, &imported_names, &fragment_digest);
        fragment_parser.input (exported_fragment_stream.str ()).eof ();
        ASSERT_FALSE (fragment_parser.has_error ())
            << "JSON error was: " << fragment_parser.last_error ().message () << ' '
            << fragment_parser.coordinate () << '\n'
            << exported_fragment_stream.str ();
    }
    {
        auto const fragment_index =
            pstore::index::get_index<pstore::trailer::indices::fragment> (import_db_);
        auto compilation_parser = import_compilation_parser (&transaction, &imported_names,
                                                             fragment_index, compilation_digest);
        compilation_parser.input (exported_compilation_stream.str ()).eof ();
        ASSERT_FALSE (compilation_parser.has_error ())
            << "JSON error was: " << compilation_parser.last_error ().message () << ' '
            << compilation_parser.coordinate () << '\n'
            << exported_compilation_stream.str ();
    }

    // Everything is now imported. Let's check what the resulting compilation record looks like.
    auto const compilation_index =
        pstore::index::get_index<pstore::trailer::indices::compilation> (import_db_);
    auto const pos = compilation_index->find (import_db_, compilation_digest);
    ASSERT_NE (pos, compilation_index->end (import_db_))
        << "Compilation was not found in the index after import";

    auto const compilation = import_db_.getro (pos->second);
    EXPECT_EQ (load_string (import_db_, compilation->triple ()), triple);
    ASSERT_EQ (compilation->size (), 2U);
    {
        pstore::repo::definition const & def1 = (*compilation)[0];
        EXPECT_EQ (def1.digest, fragment_digest);
        EXPECT_EQ (load_string (import_db_, def1.name), name1);
        EXPECT_EQ (def1.linkage (), pstore::repo::linkage::external);
        EXPECT_EQ (def1.visibility (), pstore::repo::visibility::hidden_vis);
    }
    {
        pstore::repo::definition const & def2 = (*compilation)[1];
        EXPECT_EQ (def2.digest, fragment_digest);
        EXPECT_EQ (load_string (import_db_, def2.name), name2);
        EXPECT_EQ (def2.linkage (), pstore::repo::linkage::link_once_any);
        EXPECT_EQ (def2.visibility (), pstore::repo::visibility::default_vis);
    }

    transaction.commit ();
}

TEST_F (ExchangeCompilation, MissingTriple) {
    auto const compilation = R"({ "definitions": [] })"s;
    constexpr auto compilation_digest = pstore::index::digest{0x12345678, 0x9ABCDEF0};

    pstore::exchange::import_ns::string_mapping imported_names;

    mock_mutex mutex;
    auto transaction = begin (import_db_, transaction_lock{mutex});

    auto const fragment_index =
        pstore::index::get_index<pstore::trailer::indices::fragment> (import_db_);
    auto compilation_parser = import_compilation_parser (&transaction, &imported_names,
                                                         fragment_index, compilation_digest);
    compilation_parser.input (compilation).eof ();
    EXPECT_EQ (compilation_parser.last_error (),
               make_error_code (pstore::exchange::import_ns::error::incomplete_compilation_object));

    transaction.commit ();
}

TEST_F (ExchangeCompilation, MissingDefinitions) {
    auto const names = R"([ "triple" ])"s;
    auto const compilation = R"({ "triple": 0 })"s;
    constexpr auto compilation_digest = pstore::index::digest{0x12345678, 0x9ABCDEF0};

    pstore::exchange::import_ns::string_mapping imported_names;

    mock_mutex mutex;
    auto transaction = begin (import_db_, transaction_lock{mutex});
    auto name_parser = import_strings_parser (&transaction, &imported_names);
    name_parser.input (names).eof ();
    ASSERT_FALSE (name_parser.has_error ())
        << "JSON error was: " << name_parser.last_error ().message () << ' '
        << name_parser.coordinate () << '\n'
        << names;

    auto const fragment_index =
        pstore::index::get_index<pstore::trailer::indices::fragment> (import_db_);
    auto compilation_parser = import_compilation_parser (&transaction, &imported_names,
                                                         fragment_index, compilation_digest);
    compilation_parser.input (compilation).eof ();
    EXPECT_EQ (compilation_parser.last_error (),
               make_error_code (pstore::exchange::import_ns::error::incomplete_compilation_object));

    transaction.commit ();
}
