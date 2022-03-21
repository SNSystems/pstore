//===- unittests/exchange/test_terminal.cpp -------------------------------===//
//*  _                      _             _  *
//* | |_ ___ _ __ _ __ ___ (_)_ __   __ _| | *
//* | __/ _ \ '__| '_ ` _ \| | '_ \ / _` | | *
//* | ||  __/ |  | | | | | | | | | | (_| | | *
//*  \__\___|_|  |_| |_| |_|_|_| |_|\__,_|_| *
//*                                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_terminals.hpp"

// 3rd party includes
#include <gmock/gmock.h>

// pstore includes
#include "pstore/exchange/import_error.hpp"
#include "pstore/json/json.hpp"

// Local includes
#include "empty_store.hpp"

using namespace pstore::exchange::import_ns;

namespace {

    class RuleTest : public testing::Test {
    public:
        RuleTest ()
                : db_storage_{}
                , db_{db_storage_.file ()} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

    private:
        in_memory_store db_storage_;

    protected:
        pstore::database db_;
    };

    class ImportBool : public RuleTest {
    public:
        ImportBool () = default;

        decltype (auto) make_json_bool_parser (pstore::gsl::not_null<bool *> v) {
            return pstore::json::make_parser (callbacks::make<bool_rule> (&db_, v));
        }
    };

} // end anonymous namespace

TEST_F (ImportBool, True) {
    bool v = false;
    auto parser = make_json_bool_parser (&v);
    parser.input ("true");
    parser.eof ();

    // Check the result.
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    EXPECT_EQ (v, true);
}

TEST_F (ImportBool, False) {
    bool v = true;
    auto parser = make_json_bool_parser (&v);
    parser.input ("false");
    parser.eof ();

    // Check the result.
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    EXPECT_EQ (v, false);
}

namespace {

    class ImportInt64 : public RuleTest {
    public:
        ImportInt64 () = default;
        decltype (auto) make_json_int64_parser (pstore::gsl::not_null<std::int64_t *> v) {
            return pstore::json::make_parser (callbacks::make<int64_rule> (&db_, v));
        }
    };

} // end anonymous namespace

TEST_F (ImportInt64, Zero) {
    auto v = std::int64_t{0};
    auto parser = make_json_int64_parser (&v);
    parser.input ("0");
    parser.eof ();

    // Check the result.
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    EXPECT_EQ (v, std::int64_t{0});
}

TEST_F (ImportInt64, One) {
    auto v = std::int64_t{0};
    auto parser = make_json_int64_parser (&v);
    parser.input ("1");
    parser.eof ();

    // Check the result.
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    EXPECT_EQ (v, std::int64_t{1});
}

TEST_F (ImportInt64, NegativeOne) {
    auto v = std::int64_t{0};
    auto parser = make_json_int64_parser (&v);
    parser.input ("-1");
    parser.eof ();

    // Check the result.
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    EXPECT_EQ (v, std::int64_t{-1});
}

TEST_F (ImportInt64, Min) {
    auto v = std::int64_t{0};
    auto parser = make_json_int64_parser (&v);
    auto const expected = std::numeric_limits<std::int64_t>::min ();
    parser.input (std::to_string (expected));
    parser.eof ();

    // Check the result.
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    EXPECT_EQ (v, expected);
}

TEST_F (ImportInt64, Max) {
    auto v = std::int64_t{0};
    auto parser = make_json_int64_parser (&v);
    auto const expected = std::numeric_limits<std::int64_t>::max ();
    parser.input (std::to_string (expected));
    parser.eof ();

    // Check the result.
    ASSERT_FALSE (parser.has_error ()) << "JSON error was: " << parser.last_error ().message ();
    EXPECT_EQ (v, expected);
}

// Test for max int64+1. Note that we're not trying to test the JSON parser itself here which should
// independently reject values < min int64.
TEST_F (ImportInt64, ErrorOnMaxPlus1) {
    auto v = std::int64_t{0};
    auto parser = make_json_int64_parser (&v);
    auto const expected =
        static_cast<std::uint64_t> (std::numeric_limits<std::int64_t>::max ()) + 1U;
    parser.input (std::to_string (expected));
    parser.eof ();

    // Check the result.
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (), make_error_code (error::number_too_large));
}
