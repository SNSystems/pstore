//*  _                    _                *
//* | |__   ___  __ _  __| | ___ _ __ ___  *
//* | '_ \ / _ \/ _` |/ _` |/ _ \ '__/ __| *
//* | | | |  __/ (_| | (_| |  __/ |  \__ \ *
//* |_| |_|\___|\__,_|\__,_|\___|_|  |___/ *
//*                                        *
//===- unittests/http/test_headers.cpp ------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
#include "pstore/http/headers.hpp"

#include <gtest/gtest.h>

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
    expected.websocket_key = just (std::string{"dGhlIHNhbXBsZSBub25jZQ=="});
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
