//*  _                    _                *
//* | |__   ___  __ _  __| | ___ _ __ ___  *
//* | '_ \ / _ \/ _` |/ _` |/ _ \ '__/ __| *
//* | | | |  __/ (_| | (_| |  __/ |  \__ \ *
//* |_| |_|\___|\__,_|\__,_|\___|_|  |___/ *
//*                                        *
//===- unittests/http/test_headers.cpp ------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/http/headers.hpp"

#include <gtest/gtest.h>

using namespace std::string_literals;
using pstore::just;
using pstore::httpd::header_info;

TEST (Headers, Conventional) {
    header_info const hi = header_info ()
                               .handler ("accept", "*/*")
                               .handler ("referer", " http://localhost:8000/")
                               .handler ("host", "localhost:8080")
                               .handler ("accept-Encoding", "gzip, deflate")
                               .handler ("connection", "keep-alive");
    EXPECT_EQ (hi, header_info ());
}

TEST (Headers, ExampleWS) {
    header_info const hi = header_info ()
                               .handler ("host", "example:8000")
                               .handler ("upgrade", "websocket")
                               .handler ("connection", "Upgrade")
                               .handler ("sec-websocket-key", "dGhlIHNhbXBsZSBub25jZQ==")
                               .handler ("sec-websocket-version", "13");

    header_info expected;
    expected.upgrade_to_websocket = true;
    expected.connection_upgrade = true;
    expected.websocket_key = just ("dGhlIHNhbXBsZSBub25jZQ=="s);
    expected.websocket_version = just (13U);
    EXPECT_EQ (hi, expected);
}

TEST (Headers, ConnectionCommaSeparatedList) {
    header_info const hi = header_info ().handler ("connection", "Keep-Alive, Upgrade");
    header_info expected;
    expected.connection_upgrade = true;
    EXPECT_EQ (hi, expected);
}

TEST (Headers, ExampleWSCaseInsensitive) {
    header_info const hi =
        header_info ().handler ("upgrade", "WEBSOCKET").handler ("connection", "UPGRADE");

    header_info expected;
    expected.upgrade_to_websocket = true;
    expected.connection_upgrade = true;
    EXPECT_EQ (hi, expected);
}
