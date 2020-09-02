#include <iostream>
//*                            _ _       _   _              *
//*   ___ ___  _ __ ___  _ __ (_) | __ _| |_(_) ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ \| | |/ _` | __| |/ _ \| '_ \  *
//* | (_| (_) | | | | | | |_) | | | (_| | |_| | (_) | | | | *
//*  \___\___/|_| |_| |_| .__/|_|_|\__,_|\__|_|\___/|_| |_| *
//*                     |_|                                 *
//===- unittests/exchange/test_compilation.cpp ----------------------------===//
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
#include "pstore/exchange/export_compilation.hpp"
#include "pstore/exchange/import_compilation.hpp"

// Standard library
#include <array>
#include <sstream>
#include <unordered_map>

// 3rd party includes
#include <gtest/gtest.h>

// pstore includes
#include "pstore/exchange/export_names.hpp"
#include "pstore/exchange/import_names_array.hpp"
#include "pstore/json/json.hpp"
#include "pstore/mcrepo/fragment.hpp"

// local includes
#include "add_export_strings.hpp"

namespace {

    using transaction_lock = std::unique_lock<mock_mutex>;
    using transaction = pstore::transaction<transaction_lock>;


    template <typename ImportRule, typename... Args>
    decltype (auto) make_json_object_parser (Args &&... args) {
        using rule = pstore::exchange::object_rule<ImportRule, Args...>;
        return pstore::json::make_parser (
            pstore::exchange::callbacks::make<rule> (std::forward<Args> (args)...));
    }


    // Parse the exported names JSON. The resulting index-to-string mappings are then available via
    // imported_names.
    decltype (auto) import_name_parser (transaction & transaction,
                                        pstore::exchange::import_name_mapping * const names) {
        // Create matching names in the imported database.
        using names_array_rule = pstore::exchange::names_array_members<transaction_lock>;
        return pstore::json::make_parser (
            pstore::exchange::callbacks::make<pstore::exchange::array_rule<
                names_array_rule, names_array_rule::transaction_pointer,
                names_array_rule::names_pointer>> (&transaction, names));
    }

    decltype (auto) import_compilation_parser (
        transaction & transaction, pstore::exchange::import_name_mapping * names,
        std::shared_ptr<pstore::index::fragment_index> const & fragment_index,
        pstore::index::digest const & digest) {

        using rule = pstore::exchange::import_compilation<transaction_lock>;
        return pstore::json::make_parser (
            pstore::exchange::callbacks::make<rule> (&transaction, names, fragment_index, digest));
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


    auto build_fragment (transaction & transaction, pstore::index::digest const & digest)
        -> pstore::extent<pstore::repo::fragment> {
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

} // end anonymous namespace



TEST_F (ExchangeCompilation, Empty) {
    // Add names to the store so that external fixups can use then. add_export_strings()
    // yields a mapping from each name to its indirect-address.
    constexpr auto * name = "name";
    constexpr auto * path = "path";
    constexpr auto * triple = "triple";

    std::array<pstore::gsl::czstring, 3> names{{triple, path, name}};
    std::unordered_map<std::string, pstore::typed_address<pstore::indirect_string>> indir_strings;
    add_export_strings (export_db_, std::begin (names), std::end (names),
                        std::inserter (indir_strings, std::end (indir_strings)));

    // Write the names that we just created as JSON.
    pstore::exchange::export_name_mapping exported_names;
    std::ostringstream exported_names_stream;
    export_names (exported_names_stream, export_db_, export_db_.get_current_revision (),
                  &exported_names);

    constexpr pstore::index::digest compilation_digest{0x12345678, 0x9ABCDEF0};
    constexpr pstore::index::digest fragment_digest{0x9ABCDEF0, 0x12345678};

    std::ostringstream exported_compilation_stream;

    {
        mock_mutex mutex;
        auto transaction = begin (export_db_, transaction_lock{mutex});
        pstore::extent<pstore::repo::fragment> const fext =
            build_fragment (transaction, fragment_digest);

        std::vector<pstore::repo::compilation_member> definitions{
            {fragment_digest, fext, indir_strings[name], pstore::repo::linkage::external,
             pstore::repo::visibility::hidden_vis},
        };
        pstore::extent<pstore::repo::compilation> const compilation =
            pstore::repo::compilation::alloc (transaction, indir_strings[path],
                                              indir_strings[triple], std::begin (definitions),
                                              std::end (definitions));

        pstore::exchange::export_compilation (exported_compilation_stream, export_db_,
                                              *export_db_.getro (compilation), exported_names);
        std::cout << exported_compilation_stream.str () << '\n';

        transaction.commit ();
    }



    mock_mutex mutex;
    using transaction_lock = std::unique_lock<mock_mutex>;
    auto transaction = begin (import_db_, transaction_lock{mutex});


    pstore::exchange::import_name_mapping imported_names;
    {
        auto parser = import_name_parser (transaction, &imported_names);
        parser.input (exported_names_stream.str ()).eof ();
        ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    }

    //    import_names (pstore::database & db, std::string const & exported_names, &imported_names);

    {
        mock_mutex mutex;
        auto transaction = begin (import_db_, transaction_lock{mutex});

        auto fragment_index =
            pstore::index::get_index<pstore::trailer::indices::fragment> (export_db_);
        auto parser = import_compilation_parser (transaction, &imported_names, fragment_index,
                                                 fragment_digest);
        parser.input (exported_compilation_stream.str ()).eof ();

        ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    }
}



#if 0
namespace {

    class ExchangeCompilations : public testing::Test {
    public:
        ExchangeCompilations ()
                : export_db_{export_store_.file ()}
                , import_db_{import_store_.file ()} {
            export_db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
            import_db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

        using transaction_lock = std::unique_lock<mock_mutex>;

        InMemoryStore export_store_;
        pstore::database export_db_;

        InMemoryStore import_store_;
        pstore::database import_db_;
    };

} // end anonymous namespace

TEST_F (ExchangeCompilations, Empty) {

    // Add names to the store so that external fixups can use then. add_export_strings()
    // yields a mapping from each name to its indirect-address.
    constexpr auto * name1 = "name1";
    constexpr auto * name2 = "name2";
    std::array<pstore::gsl::czstring, 2> names{{name1, name2}};
    std::unordered_map<std::string, pstore::typed_address<pstore::indirect_string>> indir_strings;
    add_export_strings (export_db_, std::begin (names), std::end (names),
                        std::inserter (indir_strings, std::end (indir_strings)));


    // Write the names that we just created as JSON.
    pstore::exchange::export_name_mapping exported_names;
    std::ostringstream exported_names_stream;
    export_names (exported_names_stream, export_db_, export_db_.get_current_revision (),
                  &exported_names);



    std::ostringstream exported_compilations_stream;
    pstore::exchange::export_compilation_index (exported_compilations_stream,
                                           export_db_,
                                           export_db_.get_current_revision (),
                                           exported_names);

#    if 0
            if (s == "compilations") {
                return push_object_rule<import_compilations_index<TransactionLock>> (
                    this, &transaction_, cnames);
            }

    pstore::exchange::import_compilation<<#typename TransactionLock#>>
#    endif

}


TEST_F (ExchangeCompilations, SingleExternal) {

    // Add names to the store so that external fixups can use then. add_export_strings()
    // yields a mapping from each name to its indirect-address.
    constexpr auto * name = "name";
    constexpr auto * path = "path";
    constexpr auto * triple = "triple";

    std::array<pstore::gsl::czstring, 3> names{{triple, path, name}};
    std::unordered_map<std::string, pstore::typed_address<pstore::indirect_string>> indir_strings;
    add_export_strings (export_db_, std::begin (names), std::end (names),
                        std::inserter (indir_strings, std::end (indir_strings)));

    // Write the names that we just created as JSON.
    pstore::exchange::export_name_mapping exported_names;
    std::ostringstream exported_names_stream;
    export_names (exported_names_stream, export_db_, export_db_.get_current_revision (),
                  &exported_names);

constexpr pstore::index::digest compilation_digest{0x12345678, 0x9ABCDEF0};
constexpr pstore::index::digest fragment_digest{0x9ABCDEF0, 0x12345678};

    {
        mock_mutex mutex;
        auto transaction = begin (export_db_, transaction_lock{mutex});
        pstore::extent<pstore::repo::fragment> const fext = build_fragment (transaction);
        auto fragment_index = pstore::index::get_index<pstore::trailer::indices::fragment>(export_db_);
        fragment_index->insert (transaction, std::make_pair (fragment_digest, fext));

        std::vector<pstore::repo::compilation_member> definitions{
            {fragment_digest, fext, indir_strings[name], pstore::repo::linkage::external, pstore::repo::visibility::hidden_vis},
        };
        pstore::extent<pstore::repo::compilation> const compilation = pstore::repo::compilation::alloc (transaction, indir_strings[path], indir_strings[triple], std::begin (definitions), std::end (definitions));

        auto compilation_index = pstore::index::get_index<pstore::trailer::indices::compilation>(export_db_);
        compilation_index->insert (transaction, std::make_pair (compilation_digest, compilation));

        transaction.commit ();
    }




    std::ostringstream exported_compilations_stream;
    pstore::exchange::export_compilation_index (exported_compilations_stream,
                                           export_db_,
                                           export_db_.get_current_revision (),
                                           exported_names);

std::cout << exported_compilations_stream.str () << '\n';
}

#endif
