//*                                 _    *
//*  _ __ ___  __ _ _   _  ___  ___| |_  *
//* | '__/ _ \/ _` | | | |/ _ \/ __| __| *
//* | | |  __/ (_| | |_| |  __/\__ \ |_  *
//* |_|  \___|\__, |\__,_|\___||___/\__| *
//*              |_|                     *
//===- unittests/httpd/test_request.cpp -----------------------------------===//
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
#include "pstore/httpd/request.hpp"

#include <functional>

#include <gmock/gmock.h>

#include "pstore/support/gsl.hpp"
#include "pstore/httpd/buffered_reader.hpp"

#include "buffered_reader_mocks.hpp"

using pstore::error_or;
using pstore::httpd::make_buffered_reader;
using pstore::httpd::request_info;
using testing::_;
using testing::Invoke;

TEST (Request, Empty) {
    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());
    error_or<std::pair<int, request_info>> res = read_request (br, io);
    ASSERT_TRUE (res.has_error ());
}

TEST (Request, Complete) {
    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));
    EXPECT_CALL (r, fill (0, _)).WillOnce (Invoke (yield_string ("GET /uri HTTP/1.1")));

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());
    error_or<std::pair<int, request_info>> res = read_request (br, io);
    ASSERT_FALSE (res.has_error ());
    auto const & request = std::get<1> (res.get_value ());
    EXPECT_EQ (request.method (), "GET");
    EXPECT_EQ (request.uri (), "/uri");
    EXPECT_EQ (request.version (), "HTTP/1.1");
}

TEST (Request, Partial) {
    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));
    EXPECT_CALL (r, fill (0, _)).WillOnce (Invoke (yield_string ("METHOD")));

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());
    error_or<std::pair<int, request_info>> res = read_request (br, io);
    ASSERT_TRUE (res.has_error ());
}
