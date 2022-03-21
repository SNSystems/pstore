//===- unittests/exchange/test_fragment.cpp -------------------------------===//
//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <string>

// 3rd party includes
#include <gtest/gtest.h>

// pstore includes
#include "pstore/exchange/import_fragment.hpp"
#include "pstore/json/json.hpp"

// local includes
#include "empty_store.hpp"

using namespace std::literals::string_literals;

namespace {

    using transaction_lock = std::unique_lock<mock_mutex>;
    using transaction = pstore::transaction<transaction_lock>;

    template <typename ImportRule, typename... Args>
    auto make_json_object_parser (pstore::database * const db, Args... args)
        -> pstore::json::parser<pstore::exchange::import_ns::callbacks> {
        using namespace pstore::exchange::import_ns;
        return pstore::json::make_parser (
            callbacks::make<object_rule<ImportRule, Args...>> (db, args...));
    }

    decltype (auto)
    import_fragment_parser (transaction * const transaction,
                            pstore::exchange::import_ns::string_mapping * const names,
                            pstore::index::digest const * const digest) {
        return make_json_object_parser<pstore::exchange::import_ns::fragment_sections> (
            &transaction->db (), transaction, names, digest);
    }

    class ImportFragment : public testing::Test {
    public:
        ImportFragment ()
                : import_db_{import_store_.file ()} {
            import_db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

        in_memory_store import_store_;
        pstore::database import_db_;
    };

} // end anonymous namespace

TEST_F (ImportFragment, BadInternalFixupTargetSection) {
    using namespace pstore::exchange::import_ns;

    // This fragment contains a text section only but it has an internal fixup that targets the
    // data section. The fragment should be rejected.
    auto const input = R"({
        "text": {
            "data":"",
            "ifixups":[ { "section":"data", "type":1, "offset":0, "addend":0 } ]
        }
    })"s;

    mock_mutex mutex;
    auto transaction = begin (import_db_, transaction_lock{mutex});

    constexpr pstore::index::digest fragment_digest{0x11111111, 0x11111111};
    string_mapping imported_names;

    auto parser = import_fragment_parser (&transaction, &imported_names, &fragment_digest);
    parser.input (input).eof ();
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (), make_error_code (error::internal_fixup_target_not_found));
}
