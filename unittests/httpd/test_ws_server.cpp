//*                                               *
//* __      _____   ___  ___ _ ____   _____ _ __  *
//* \ \ /\ / / __| / __|/ _ \ '__\ \ / / _ \ '__| *
//*  \ V  V /\__ \ \__ \  __/ |   \ V /  __/ |    *
//*   \_/\_/ |___/ |___/\___|_|    \_/ \___|_|    *
//*                                               *
//===- unittests/httpd/test_ws_server.cpp ---------------------------------===//
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
#include "pstore/httpd/ws_server.hpp"

#include <gmock/gmock.h>

#include "buffered_reader_mocks.hpp"

using testing::_;
using testing::Invoke;

using pstore::httpd::make_buffered_reader;

namespace {}


TEST (WsServer, NothingFromClient) {
    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));

    std::vector<std::uint8_t> output;

    auto sender = [&output](int io, pstore::gsl::span<std::uint8_t const> const & s) {
        std::copy (s.begin (), s.end (), std::back_inserter (output));
        return pstore::error_or<int>{pstore::in_place, io + 1};
    };

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());

    ws_server_loop (br, sender, io);

    // a close frame with error 0x3ee (1006: abnormal closure).
    EXPECT_THAT (output, ::testing::ElementsAre (std::uint8_t{0x88}, std::uint8_t{0x02},
                                                 std::uint8_t{0x03}, std::uint8_t{0xee}));
}

struct frame_and_mask {
    pstore::httpd::frame_fixed_layout frame;
    std::array<std::uint8_t, 4> mask;
};

TEST (WsServer, Ping) {
    using namespace pstore::gsl;

    refiller r;
    std::vector<std::uint8_t> send_frames;
    {
        pstore::httpd::frame_fixed_layout sf1{};
        sf1.mask = true;
        sf1.opcode = static_cast<std::uint16_t> (pstore::httpd::opcode::ping);
        sf1.fin = true;
        sf1 = pstore::httpd::host_to_network (sf1);
        auto const sf1_span = as_bytes (make_span (&sf1, 1));
        std::copy (std::begin (sf1_span), std::end (sf1_span), std::back_inserter (send_frames));
        // 4 mask bytes (all 0)
        std::generate_n (std::back_inserter (send_frames), 4, []() { return std::uint8_t{0}; });
    }
    {
        pstore::httpd::frame_fixed_layout sf2{};
        sf2.mask = true;
        sf2.opcode = static_cast<std::uint16_t> (pstore::httpd::opcode::close);
        sf2.fin = true;
        sf2 = pstore::httpd::host_to_network (sf2);
        auto const sf2_span = as_bytes (make_span (&sf2, 1));
        std::copy (std::begin (sf2_span), std::end (sf2_span), std::back_inserter (send_frames));
        // 4 mask bytes (all 0)
        std::generate_n (std::back_inserter (send_frames), 4, []() { return std::uint8_t{0}; });
    }


    std::vector<std::uint8_t> expected_frames;
    {
        pstore::httpd::frame_fixed_layout xf1{};
        xf1.mask = false;
        xf1.opcode = static_cast<std::uint16_t> (pstore::httpd::opcode::pong);
        xf1.fin = true;
        xf1 = pstore::httpd::host_to_network (xf1);
        auto const xf1_span = as_bytes (make_span (&xf1, 1));
        std::copy (std::begin (xf1_span), std::end (xf1_span),
                   std::back_inserter (expected_frames));
    }
    {
        pstore::httpd::frame_fixed_layout xf2{};
        xf2.payload_length = sizeof (std::uint16_t); // payload is the close status code.
        xf2.mask = false;
        xf2.opcode = static_cast<std::uint16_t> (pstore::httpd::opcode::close);
        xf2.fin = true;
        xf2 = pstore::httpd::host_to_network (xf2);
        auto const xf2_span = as_writeable_bytes (make_span (&xf2, 1));
        std::copy (std::begin (xf2_span), std::end (xf2_span),
                   std::back_inserter (expected_frames));
        auto const close_code = pstore::httpd::host_to_network (
            static_cast<std::uint16_t> (pstore::httpd::close_status_code::normal));
        auto const close_span = as_bytes (make_span (&close_code, 1));
        std::copy (std::begin (close_span), std::end (close_span),
                   std::back_inserter (expected_frames));
    }



    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));
    EXPECT_CALL (r, fill (0, _))
        .WillOnce (Invoke (yield_bytes (as_bytes (make_span (send_frames)))));

    std::vector<std::uint8_t> output;

    auto sender = [&output](int io, pstore::gsl::span<std::uint8_t const> const & s) {
        std::copy (s.begin (), s.end (), std::back_inserter (output));
        return pstore::error_or<int>{pstore::in_place, io + 1};
    };

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());

    ws_server_loop (br, sender, io);

    EXPECT_THAT (pstore::gsl::make_span (output),
                 ::testing::ContainerEq (make_span (expected_frames)));
}
