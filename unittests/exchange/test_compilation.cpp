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
#include "pstore/mcrepo/fragment.hpp"

// local includes
#include "add_export_strings.hpp"

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
    pstore::exchange::export_compilations (exported_compilations_stream,
                                           export_db_,
                                           export_db_.get_current_revision (),
                                           exported_names);

#if 0
            if (s == "compilations") {
                return push_object_rule<import_compilations_index<TransactionLock>> (
                    this, &transaction_, cnames);
            }

    pstore::exchange::import_compilation<<#typename TransactionLock#>>
#endif

}


namespace {

    using transaction_lock = std::unique_lock<mock_mutex>;
    using transaction = pstore::transaction<transaction_lock>;

    auto build_fragment (transaction & transaction) -> pstore::extent<pstore::repo::fragment> {
        pstore::repo::section_content content {pstore::repo::section_kind::bss};
        content.align = 4U;
        content.data.resize (128U);

        std::array<pstore::repo::bss_section_creation_dispatcher, 1> dispatcher;
        dispatcher[0].set_content(&content);
        return pstore::repo::fragment::alloc (transaction, std::begin (dispatcher), std::end (dispatcher));
    }

}

TEST_F (ExchangeCompilations, SingleExternal) {

    // Add names to the store so that external fixups can use then. add_export_strings()
    // yields a mapping from each name to its indirect-address.
    constexpr auto * name = "name";
    constexpr auto * path = "path";
    constexpr auto * triple = "triple";

    std::array<pstore::gsl::czstring, 1> names{{name}};
    std::unordered_map<std::string, pstore::typed_address<pstore::indirect_string>> indir_strings;
    add_export_strings (export_db_, std::begin (names), std::end (names),
                        std::inserter (indir_strings, std::end (indir_strings)));

    {
        mock_mutex mutex;
        auto transaction = begin (export_db_, transaction_lock{mutex});
        pstore::extent<pstore::repo::fragment> const fext = build_fragment (transaction);
        auto fragment_index = pstore::index::get_index<pstore::trailer::indices::fragment>(export_db_);


        std::vector<pstore::repo::compilation_member> v{
            {pstore::index::digest{28U}, fext, indir_strings[name], pstore::repo::linkage::external, pstore::repo::visibility::hidden_vis},
        };
        auto compilation = pstore::repo::compilation::alloc (transaction, indir_strings[path], indir_strings[triple], std::begin (v), std::end (v));

        auto compilation_index = pstore::index::get_index<pstore::trailer::indices::compilation>(export_db_);

        transaction.commit ();
    }


    // Write the names that we just created as JSON.
    pstore::exchange::export_name_mapping exported_names;
    std::ostringstream exported_names_stream;
    export_names (exported_names_stream, export_db_, export_db_.get_current_revision (),
                  &exported_names);


    std::ostringstream exported_compilations_stream;
    pstore::exchange::export_compilations (exported_compilations_stream,
                                           export_db_,
                                           export_db_.get_current_revision (),
                                           exported_names);

}

