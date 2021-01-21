//*                                               *
//* __      _____   ___  ___ _ ____   _____ _ __  *
//* \ \ /\ / / __| / __|/ _ \ '__\ \ / / _ \ '__| *
//*  \ V  V /\__ \ \__ \  __/ |   \ V /  __/ |    *
//*   \_/\_/ |___/ |___/\___|_|    \_/ \___|_|    *
//*                                               *
//===- unittests/http/test_ws_server.cpp ----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/http/ws_server.hpp"

// 3rd part includes
#include <gmock/gmock.h>
// pstore includes
#include "pstore/http/block_for_input.hpp"
// local includes
#include "buffered_reader_mocks.hpp"

using testing::_;
using testing::Invoke;

using pstore::httpd::make_buffered_reader;


namespace pstore {
    namespace httpd {

        /// A specialization of block_for_input() which always returns a result indicating that
        /// data is available on the input socket.
        template <typename Reader>
        inputs_ready block_for_input (Reader const &, int, pipe_descriptor const *) {
            return {true, false};
        }

    } // end namespace httpd
} // end namespace pstore


TEST (WsServer, NothingFromClient) {
    // Define what the client will send to the server (just EOF).
    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));

    // Record the server's response.
    std::vector<std::uint8_t> output;
    auto sender = [&output] (int io, pstore::gsl::span<std::uint8_t const> const & s) {
        std::copy (s.begin (), s.end (), std::back_inserter (output));
        return pstore::error_or<int>{pstore::in_place, io + 1};
    };

    auto io = 0;
    auto br = make_buffered_reader<decltype (io)> (r.refill_function ());
    ws_server_loop (br, sender, io, "", pstore::httpd::channel_container{});

    // A close frame with error 0x3ee (1006: abnormal closure).
    EXPECT_THAT (output, ::testing::ElementsAre (std::uint8_t{0x88}, std::uint8_t{0x02},
                                                 std::uint8_t{0x03}, std::uint8_t{0xee}));
}

struct frame_and_mask {
    pstore::httpd::frame_fixed_layout frame;
    std::array<std::uint8_t, 4> mask;
};

TEST (WsServer, Ping) {
    using namespace pstore::gsl;

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
        std::generate_n (std::back_inserter (send_frames), 4, [] () { return std::uint8_t{0}; });
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
        std::generate_n (std::back_inserter (send_frames), 4, [] () { return std::uint8_t{0}; });
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
        xf2.payload_length =
            std::uint16_t{sizeof (std::uint16_t)}; // payload is the close status code.
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


    // Define what the client will send to the server (the contents of send_frames above).
    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));
    EXPECT_CALL (r, fill (0, _))
        .WillOnce (Invoke (yield_bytes (as_bytes (make_span (send_frames)))));
    auto io = 0;
    auto br = make_buffered_reader<decltype (io)> (r.refill_function ());

    // Record the server's response.
    std::vector<std::uint8_t> output;
    auto sender = [&output] (int io2, pstore::gsl::span<std::uint8_t const> const & s) {
        std::copy (s.begin (), s.end (), std::back_inserter (output));
        return pstore::error_or<int>{pstore::in_place, io2 + 1};
    };


    ws_server_loop (br, sender, io, std::string{} /*uri*/, pstore::httpd::channel_container{});

    EXPECT_THAT (pstore::gsl::make_span (output),
                 ::testing::ContainerEq (make_span (expected_frames)));
}
