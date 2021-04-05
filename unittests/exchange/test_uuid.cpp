//===- unittests/exchange/test_uuid.cpp -----------------------------------===//
//*              _     _  *
//*  _   _ _   _(_) __| | *
//* | | | | | | | |/ _` | *
//* | |_| | |_| | | (_| | *
//*  \__,_|\__,_|_|\__,_| *
//*                       *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_uuid.hpp"

#include <gtest/gtest.h>

#include "pstore/exchange/import_error.hpp"
#include "pstore/json/json.hpp"

#include "empty_store.hpp"

using namespace std::string_literals;

namespace {

    class Uuid : public ::testing::Test {
    public:
        Uuid ()
                : db_storage_{}
                , db_{db_storage_.file ()} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

    private:
        InMemoryStore db_storage_;

    protected:
        pstore::database db_;

        decltype (auto) make_json_uuid_parser (pstore::gsl::not_null<pstore::uuid *> v) {
            using namespace pstore::exchange::import_ns;
            return pstore::json::make_parser (callbacks::make<uuid_rule> (&db_, v));
        }
    };

} // end anonymous namespace

TEST_F (Uuid, Good) {
    auto const input = R"("84949cc5-4701-4a84-895b-354c584a981b")"s;
    constexpr auto expected = pstore::uuid{
        pstore::uuid::container_type{{0x84, 0x94, 0x9c, 0xc5, 0x47, 0x01, 0x4a, 0x84, 0x89, 0x5b,
                                      0x35, 0x4c, 0x58, 0x4a, 0x98, 0x1b}}};

    pstore::uuid id;
    auto parser = make_json_uuid_parser (&id);
    parser.input (input).eof ();

    ASSERT_FALSE (parser.has_error ())
        << "Expected the JSON parse to succeed (" << parser.last_error ().message () << ')';
    EXPECT_EQ (id, expected);
}

TEST_F (Uuid, Bad) {
    pstore::uuid id;
    auto parser = make_json_uuid_parser (&id);
    parser.input (R"("bad")"s).eof ();
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (),
               make_error_code (pstore::exchange::import_ns::error::bad_uuid));
}
