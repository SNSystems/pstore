//===- unittests/exchange/test_paths.cpp ----------------------------------===//
//*              _   _          *
//*  _ __   __ _| |_| |__  ___  *
//* | '_ \ / _` | __| '_ \/ __| *
//* | |_) | (_| | |_| | | \__ \ *
//* | .__/ \__,_|\__|_| |_|___/ *
//* |_|                         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/export_strings.hpp"
#include "pstore/exchange/import_strings.hpp"

// Standard library includes
#include <array>
#include <string>
#include <unordered_map>

// 3rd party includes
#include <gtest/gtest.h>

// pstore includes
#include "pstore/core/database.hpp"
#include "pstore/core/indirect_string.hpp"
#include "pstore/exchange/export_strings.hpp"
#include "pstore/exchange/import_strings_array.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/json/json.hpp"
#include "pstore/exchange/import_non_terminals.hpp"

// local includes
#include "add_export_strings.hpp"
#include "empty_store.hpp"

using namespace std::string_literals;

namespace {

    using transaction_lock = std::unique_lock<mock_mutex>;
    using transaction = pstore::transaction<transaction_lock>;

    class ExchangePaths : public testing::Test {
    public:
        ExchangePaths ()
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

    template <typename ImportRule, typename... Args>
    pstore::json::parser<pstore::exchange::import_ns::callbacks>
    make_json_array_parser (pstore::database * const db, Args... args) {
        using namespace pstore::exchange::import_ns;
        return pstore::json::make_parser (
            callbacks::make<array_rule<ImportRule, Args...>> (db, args...));
    }

    // Parse the exported strings JSON. The resulting index-to-string mappings are then available
    // via 'names'.
    decltype (auto)
    import_strings_parser (transaction * const transaction,
                           pstore::exchange::import_ns::string_mapping * const names) {
        return make_json_array_parser<pstore::exchange::import_ns::strings_array_members> (
            &transaction->db (), transaction, names);
    }

} // end anonymous namespace

TEST_F (ExchangePaths, ExportEmpty) {
    using namespace pstore::exchange::export_ns;
    static constexpr auto comments = false;
    string_mapping exported_strings{export_db_, path_index_tag ()};
    ostringstream exported_strings_stream;
    emit_strings<pstore::trailer::indices::path> (exported_strings_stream, indent{}, export_db_,
                                                  export_db_.get_current_revision (), "",
                                                  &exported_strings, comments);

    EXPECT_EQ (exported_strings_stream.str (), "");
    EXPECT_EQ (exported_strings.size (), 0U);
}

TEST_F (ExchangePaths, ImportEmpty) {
    auto const exported_paths = "[]\n"s;

    mock_mutex mutex;
    auto transaction = begin (import_db_, transaction_lock{mutex});

    pstore::exchange::import_ns::string_mapping imported_paths;
    {
        auto name_parser = import_strings_parser (&transaction, &imported_paths);
        name_parser.input (exported_paths).eof ();
        ASSERT_FALSE (name_parser.has_error ())
            << "JSON error was: " << name_parser.last_error ().message () << ' '
            << name_parser.coordinate () << '\n'
            << exported_paths;
    }

    imported_paths.flush (&transaction);
    transaction.commit ();

    EXPECT_EQ (imported_paths.lookup (0U), pstore::exchange::import_ns::error::no_such_name);
}

TEST_F (ExchangePaths, RoundTripForTwoPaths) {
    // The output from the export phase.
    static constexpr auto comments = false;
    pstore::exchange::export_ns::ostringstream exported_names_stream;

    // The export phase. We put two strings into the paths index and export it.
    {
        std::array<pstore::gsl::czstring, 2> paths{{"path1", "path2"}};
        std::unordered_map<std::string, pstore::typed_address<pstore::indirect_string>>
            indir_strings;
        add_export_strings<pstore::trailer::indices::path> (
            export_db_, std::begin (paths), std::end (paths),
            std::inserter (indir_strings, std::end (indir_strings)));

        // Write the paths that we just created as JSON.
        pstore::exchange::export_ns::string_mapping exported_names{
            export_db_, pstore::exchange::export_ns::path_index_tag ()};
        pstore::exchange::export_ns::emit_strings<pstore::trailer::indices::path> (
            exported_names_stream, pstore::exchange::export_ns::indent{}, export_db_,
            export_db_.get_current_revision (), "", &exported_names, comments);
    }

    // The output from the import phase: the mapping from path index to address.
    pstore::exchange::import_ns::string_mapping imported_names;

    // The import phase. Read the JSON produced by the export phase and populate a database
    // accordingly.
    {
        mock_mutex mutex;
        auto transaction = begin (import_db_, transaction_lock{mutex});
        {
            auto name_parser = import_strings_parser (&transaction, &imported_names);
            name_parser.input (exported_names_stream.str ()).eof ();
            ASSERT_FALSE (name_parser.has_error ())
                << "JSON error was: " << name_parser.last_error ().message () << ' '
                << name_parser.coordinate () << '\n'
                << exported_names_stream.str ();
        }
        transaction.commit ();
    }

    // Now verify the result of the import phase.
    std::list<std::string> out;
    ASSERT_EQ (imported_names.size (), 2U);

    {
        auto const str_addr_or_err = imported_names.lookup (0U);
        ASSERT_EQ (str_addr_or_err.get_error (), std::error_code{});
        pstore::shared_sstring_view owner;
        out.push_back (get_sstring_view (import_db_, *str_addr_or_err, &owner).to_string ());
    }
    {
        auto const str_addr_or_err = imported_names.lookup (1U);
        ASSERT_EQ (str_addr_or_err.get_error (), std::error_code{});
        pstore::shared_sstring_view owner;
        out.push_back (get_sstring_view (import_db_, *str_addr_or_err, &owner).to_string ());
    }

    EXPECT_THAT (out, testing::UnorderedElementsAre ("path1", "path2"));
    EXPECT_EQ (imported_names.lookup (2U), pstore::exchange::import_ns::error::no_such_name);
}
