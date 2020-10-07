//*              _     _  *
//*  _   _ _   _(_) __| | *
//* | | | | | | | |/ _` | *
//* | |_| | |_| | | (_| | *
//*  \__,_|\__,_|_|\__,_| *
//*                       *
//===- unittests/exchange/test_uuid.cpp -----------------------------------===//
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
#include "pstore/exchange/import_uuid.hpp"

#include <gtest/gtest.h>

#include "pstore/exchange/import_error.hpp"
#include "pstore/json/json.hpp"

using namespace std::string_literals;

namespace {

    decltype (auto) make_json_uuid_parser (pstore::gsl::not_null<pstore::uuid *> v) {
        return pstore::json::make_parser (
            pstore::exchange::callbacks::make<pstore::exchange::import_uuid_rule> (v));
    }

} // end anonymous namespace

TEST (UUID, Good) {
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

TEST (UUID, Bad) {
    pstore::uuid id;
    auto parser = make_json_uuid_parser (&id);
    parser.input (R"("bad")"s).eof ();
    EXPECT_TRUE (parser.has_error ());
    EXPECT_EQ (parser.last_error (), make_error_code (pstore::exchange::import_error::bad_uuid));
}
