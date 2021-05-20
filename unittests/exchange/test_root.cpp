//===- unittests/exchange/test_root.cpp -----------------------------------===//
//*                  _    *
//*  _ __ ___   ___ | |_  *
//* | '__/ _ \ / _ \| __| *
//* | | | (_) | (_) | |_  *
//* |_|  \___/ \___/ \__| *
//*                       *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_root.hpp"

// 3rd party includes
#include <gtest/gtest.h>

// pstore includes
#include "pstore/core/database.hpp"
#include "pstore/json/json.hpp"
#include "pstore/exchange/import_root.hpp"

// Local includes
#include "empty_store.hpp"

namespace {

    class ExchangeRoot : public testing::Test {
    public:
        ExchangeRoot ()
                : import_db_{import_store_.file ()} {
            import_db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

        InMemoryStore import_store_;
        pstore::database import_db_;
    };

} // end anonymous namespace


TEST_F (ExchangeRoot, ImportId) {
    using namespace pstore::exchange;

    static constexpr auto json =
        R"({ "version":1, "id":"7a73d64e-5873-439c-ac8f-2b3a68aebe53", "transactions":[] })";

    pstore::json::parser<import_ns::callbacks> parser = import_ns::create_parser (import_db_);
    parser.input (json).eof ();
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ()
                                       << ' ' << parser.coordinate () << '\n'
                                       << json;

    EXPECT_EQ (import_db_.get_header ().id (), pstore::uuid{"7a73d64e-5873-439c-ac8f-2b3a68aebe53"})
        << "The file UUID was not preserved by import";
    EXPECT_TRUE (import_db_.get_header ().is_valid ()) << "The file header was not valid";
}
