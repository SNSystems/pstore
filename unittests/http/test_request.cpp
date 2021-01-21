//*                                 _    *
//*  _ __ ___  __ _ _   _  ___  ___| |_  *
//* | '__/ _ \/ _` | | | |/ _ \/ __| __| *
//* | | |  __/ (_| | |_| |  __/\__ \ |_  *
//* |_|  \___|\__, |\__,_|\___||___/\__| *
//*              |_|                     *
//===- unittests/http/test_request.cpp ------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/http/request.hpp"

#include <functional>

#include <gmock/gmock.h>

#include "pstore/support/gsl.hpp"
#include "pstore/http/buffered_reader.hpp"

#include "buffered_reader_mocks.hpp"

using pstore::error_or;
using pstore::error_or_n;
using pstore::httpd::make_buffered_reader;
using pstore::httpd::request_info;
using testing::_;
using testing::AnyNumber;
using testing::Invoke;
using testing::Return;

TEST (Request, Empty) {
    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());
    error_or_n<int, request_info> const res = read_request (br, io);
    ASSERT_FALSE (static_cast<bool> (res));
}

TEST (Request, Complete) {
    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));
    EXPECT_CALL (r, fill (0, _)).WillOnce (Invoke (yield_string ("GET /uri HTTP/1.1")));

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());
    error_or_n<int, request_info> const res = read_request (br, io);
    ASSERT_TRUE (static_cast<bool> (res)) << "There was an error:" << res.get_error ();
    auto const & request = std::get<1> (res);
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
    error_or_n<int, request_info> const res = read_request (br, io);
    ASSERT_FALSE (static_cast<bool> (res));
}

namespace {

    class header_handler {
    public:
        virtual ~header_handler () noexcept = default;
        virtual int call (int io, std::string const & key, std::string const & value) const = 0;
    };

    class mocked_header_handler : public header_handler {
    public:
        MOCK_CONST_METHOD3 (call, int (int, std::string const &, std::string const &));
    };

} // end anonymous namespace

TEST (ReadHeaders, Common) {
    refiller r;
    EXPECT_CALL (r, fill (_, _)).Times (AnyNumber ()).WillRepeatedly (Invoke (eof ()));
    EXPECT_CALL (r, fill (0, _))
        .Times (AnyNumber ())
        .WillRepeatedly (Invoke (yield_string ("HOST: localhost:8080\r\n"
                                               "Accept-Encoding: gzip, deflate\r\n"
                                               "Referer: http://localhost:8080/\r\n"
                                               "\r\n")));

    auto br = make_buffered_reader<int> (r.refill_function ());

    mocked_header_handler handler;
    EXPECT_CALL (handler, call (0, "host", "localhost:8080")).WillOnce (Return (1));
    EXPECT_CALL (handler, call (1, "accept-encoding", "gzip, deflate")).WillOnce (Return (2));
    EXPECT_CALL (handler, call (2, "referer", "http://localhost:8080/")).WillOnce (Return (3));
    error_or_n<int, int> const res = pstore::httpd::read_headers (
        br, 0,
        [&handler] (int io, std::string const & key, std::string const & value) {
            return handler.call (io, key, value);
        },
        0);
    ASSERT_TRUE (static_cast<bool> (res)) << "There was an error:" << res.get_error ();
    EXPECT_EQ (std::get<0> (*res), 1) << "Reader state is incorrect";
    EXPECT_EQ (std::get<1> (*res), 3) << "Handler state is incorrect";
}
