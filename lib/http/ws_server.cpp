//*                                               *
//* __      _____   ___  ___ _ ____   _____ _ __  *
//* \ \ /\ / / __| / __|/ _ \ '__\ \ / / _ \ '__| *
//*  \ V  V /\__ \ \__ \  __/ |   \ V /  __/ |    *
//*   \_/\_/ |___/ |___/\___|_|    \_/ \___|_|    *
//*                                               *
//===- lib/http/ws_server.cpp ---------------------------------------------===//
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
#include "pstore/http/ws_server.hpp"

#include "pstore/support/unsigned_cast.hpp"

namespace pstore {
    namespace httpd {

        auto ws_error_category::name () const noexcept -> gsl::czstring { return "ws-error"; }

        auto ws_error_category::message (int const error) const -> std::string {
            switch (static_cast<ws_error> (error)) {
            case ws_error::reserved_bit_set:
                return "One of a client frame's reserved bits was unexpectedly set";
            case ws_error::payload_too_long: return "The frame's payload length was too large";
            case ws_error::unmasked_frame: return "The client sent an unmasked frame";
            case ws_error::message_too_long: return "Message too long";
            case ws_error::insufficient_data: return "Insufficient data was received";
            }
            return "Unknown error";
        }

        auto make_error_code (ws_error const e) -> std::error_code {
            static_assert (std::is_same<std::underlying_type<decltype (e)>::type, int>::value,
                           "base type of ws_error must be int to permit safe static cast");
            static ws_error_category const category;
            return {static_cast<int> (e), category};
        }


        auto opcode_name (opcode const op) noexcept -> gsl::czstring {
            switch (op) {
            case opcode::continuation: return "continuation";
            case opcode::text: return "text";
            case opcode::binary: return "binary";
            case opcode::reserved_nc_1: return "reserved_nc_1";
            case opcode::reserved_nc_2: return "reserved_nc_2";
            case opcode::reserved_nc_3: return "reserved_nc_3";
            case opcode::reserved_nc_4: return "reserved_nc_4";
            case opcode::reserved_nc_5: return "reserved_nc_5";
            case opcode::close: return "close";
            case opcode::ping: return "ping";
            case opcode::pong: return "pong";
            case opcode::reserved_control_1: return "reserved_control_1";
            case opcode::reserved_control_2: return "reserved_control_2";
            case opcode::reserved_control_3: return "reserved_control_3";
            case opcode::reserved_control_4: return "reserved_control_4";
            case opcode::reserved_control_5: return "reserved_control_5";
            case opcode::unknown: break;
            }
            return "unknown";
        }

        auto is_valid_close_status_code (std::uint16_t const code) noexcept -> bool {
            switch (static_cast<close_status_code> (code)) {
            case close_status_code::going_away:
            case close_status_code::internal_error:
            case close_status_code::invalid_payload:
            case close_status_code::mandatory_ext:
            case close_status_code::message_too_big:
            case close_status_code::normal:
            case close_status_code::policy_violation:
            case close_status_code::protocol_error:
            case close_status_code::unsupported_data: return true;

            case close_status_code::abnormal_closure:
            case close_status_code::invalid_response:
            case close_status_code::no_status_rcvd:
            case close_status_code::reserved:
            case close_status_code::service_restart:
            case close_status_code::tls_handshake:
            case close_status_code::try_again: return false;
            }
            return code >= 3000 && code < 5000;
        }

        error_or_n<gsl::span<std::uint8_t> const>
        details::decode_payload (std::uint64_t const expected_length,
                                 gsl::span<std::uint8_t const> const & mask,
                                 gsl::span<std::uint8_t> const & payload) {
            using return_type = error_or_n<gsl::span<std::uint8_t>>;
            auto const size = payload.size ();
            if (size < 0 || unsigned_cast (size) != expected_length) {
                return return_type{ws_error::insufficient_data};
            }

            // "Octet i of the transformed data ("transformed-octet-i") is the XOR of octet i of
            // the original data ("original-octet-i") with octet at index i modulo 4 of the
            // masking key ("masking-key-octet-j"):
            //     j                   = i MOD 4
            //     transformed-octet-i = original-octet-i XOR masking-key-octet-j"

            for (auto ctr = gsl::span<std::uint8_t>::index_type{0}; ctr < size; ++ctr) {
                payload[ctr] ^= mask[ctr % 4];
            }
            return return_type{payload};
        }

    } // end namespace httpd
} // end namespace pstore
