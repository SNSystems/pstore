//*                                               *
//* __      _____   ___  ___ _ ____   _____ _ __  *
//* \ \ /\ / / __| / __|/ _ \ '__\ \ / / _ \ '__| *
//*  \ V  V /\__ \ \__ \  __/ |   \ V /  __/ |    *
//*   \_/\_/ |___/ |___/\___|_|    \_/ \___|_|    *
//*                                               *
//===- include/pstore/http/ws_server.hpp ----------------------------------===//
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
#ifndef PSTORE_HTTP_WS_SERVER_HPP
#define PSTORE_HTTP_WS_SERVER_HPP

#include <cassert>
#include <cstdint>
#include <limits>
#include <unordered_map>

#ifdef _WIN32
#    include <winsock2.h>
#else
#    include <arpa/inet.h>
#endif //_WIN32

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/broker_intf/signal_cv.hpp"
#include "pstore/http/block_for_input.hpp"
#include "pstore/http/buffered_reader.hpp"
#include "pstore/http/endian.hpp"
#include "pstore/http/send.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/support/bit_field.hpp"
#include "pstore/support/pubsub.hpp"
#include "pstore/support/utf.hpp"


namespace pstore {
    namespace httpd {

        constexpr bool log_frame_info = false;        // Log the frame header as it is received?
        constexpr bool log_received_messages = false; // Log the text of received messages?

        // The version of the WebSockets protocol that we support.
        constexpr unsigned ws_version = 13;

        enum ws_error : int {
            reserved_bit_set = 1,
            payload_too_long,
            unmasked_frame,
            message_too_long,
            insufficient_data,
        };

        // *********************
        // * ws_error_category *
        // *********************
        class ws_error_category : public std::error_category {
        public:
            // The need for this constructor was removed by CWG defect 253 but Clang (prior
            // to 3.9.0) and GCC (before 4.6.4) require its presence.
            ws_error_category () noexcept {} // NOLINT
            auto name () const noexcept -> char const * override;
            auto message (int error) const -> std::string override;
        };


        auto make_error_code (ws_error e) -> std::error_code;

    } // end namespace httpd
} // end namespace pstore

namespace std {

    template <>
    struct is_error_code_enum<pstore::httpd::ws_error> : std::true_type {};

} // end namespace std


namespace pstore {
    namespace httpd {

        // Frame format:
        //
        //  0                   1                   2                   3
        //  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        // +-+-+-+-+-------+-+-------------+-------------------------------+
        // |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
        // |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
        // |N|V|V|V|       |S|             |   (if payload len==126/127)   |
        // | |1|2|3|       |K|             |                               |
        // +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
        // |     Extended payload length continued, if payload len == 127  |
        // + - - - - - - - - - - - - - - - +-------------------------------+
        // |                               |Masking-key, if MASK set to 1  |
        // +-------------------------------+-------------------------------+
        // | Masking-key (continued)       |          Payload Data         |
        // +-------------------------------- - - - - - - - - - - - - - - - +
        // :                     Payload Data continued ...                :
        // + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
        // |                     Payload Data continued ...                |
        // +---------------------------------------------------------------+

        union frame_fixed_layout {
            std::uint16_t raw;
            bit_field<std::uint16_t, 0, 7> payload_length;
            bit_field<std::uint16_t, 7, 1> mask;
            bit_field<std::uint16_t, 8, 4> opcode;
            bit_field<std::uint16_t, 12, 3> rsv;
            bit_field<std::uint16_t, 15, 1> fin;
        };

        template <>
        inline frame_fixed_layout
        host_to_network<frame_fixed_layout> (frame_fixed_layout x) noexcept {
            x.raw = host_to_network (x.raw);
            return x;
        }

        enum class opcode {
            continuation = 0x0,  // %x0 denotes a continuation frame
            text = 0x1,          // %x1 denotes a text frame
            binary = 0x2,        // %x2 denotes a binary frame
            reserved_nc_1 = 0x3, // %x3-7 are reserved for further non-control frames
            reserved_nc_2 = 0x4,
            reserved_nc_3 = 0x5,
            reserved_nc_4 = 0x6,
            reserved_nc_5 = 0x7,
            close = 0x8, // %x8 denotes a connection close
            ping = 0x9,  // %x9 denotes a ping
            pong = 0xA,  // %xA denotes a pong
            reserved_control_1 = 0xB,
            reserved_control_2 = 0xC,
            reserved_control_3 = 0xD,
            reserved_control_4 = 0xE,
            reserved_control_5 = 0xF,

            unknown = 0xFF,
        };

        auto opcode_name (opcode op) noexcept -> char const *;

        inline auto is_control_frame_opcode (opcode c) noexcept -> bool {
            return (static_cast<unsigned> (c) & 0x08U) != 0U;
        }


        enum class close_status_code : std::uint16_t {
            normal = 1000,           // Normal Closure
            going_away = 1001,       // Going Away
            protocol_error = 1002,   // Protocol error
            unsupported_data = 1003, // Unsupported Data
            reserved = 1004,         // Reserved
            no_status_rcvd = 1005,   // No Status Rcvd
            abnormal_closure = 1006, // Abnormal Closure
            invalid_payload = 1007,  // Invalid frame payload data
            policy_violation = 1008, // Policy Violation
            message_too_big = 1009,  // Message Too Big
            mandatory_ext = 1010,    // Mandatory Ext.
            internal_error = 1011,   // Internal Error
            service_restart = 1012,  // Service Restart
            try_again = 1013,        // Try Again Later
            invalid_response = 1014, // "The server was acting as a gateway or proxy and received an
                                     // invalid response from the upstream server. This is similar
                                     // to 502 HTTP Status Code."
            tls_handshake = 1015,    // TLS handshake
        };

        auto is_valid_close_status_code (std::uint16_t code) noexcept -> bool;


        struct frame {
            frame (std::uint16_t op_, bool fin_, std::vector<std::uint8_t> && p)
                    : op{static_cast<opcode> (static_cast<std::uint16_t> (op_))}
                    , fin{fin_}
                    , payload{std::move (p)} {}

            opcode op;
            bool fin;
            std::vector<std::uint8_t> payload;
        };



        namespace details {

            template <typename T, typename Reader, typename IO>
            error_or_n<IO, T> read_and_byte_swap (Reader & reader, IO io) {
                auto v = T{0};
                return reader.get_span (io, gsl::make_span (&v, 1)) >>=
                       [](IO io2, gsl::span<T> const & l1) {
                           return error_or_n<IO, T>{in_place, io2, network_to_host (l1.at (0))};
                       };
            }

            template <typename T, typename Reader, typename IO>
            error_or_n<IO, std::uint64_t> read_extended_payload_length (Reader & reader, IO io) {
                auto length16n = T{0};
                return reader.get_span (io, gsl::make_span (&length16n, 1)) >>=
                       [](IO io2, gsl::span<T> const & l1) {
                           return error_or_n<IO, T>{in_place, io2, network_to_host (l1.at (0))};
                       };
            }

            template <typename Reader, typename IO>
            error_or_n<IO, std::uint64_t> read_payload_length (Reader & reader, IO io,
                                                               unsigned base_length) {
                if (base_length < 126U) {
                    // "If 0-125, that is the payload length."
                    return error_or_n<IO, std::uint64_t>{in_place, io, base_length};
                }
                if (base_length == 126U) {
                    // "If 126, the following 2 bytes interpreted as a 16-bit unsigned integer are
                    // the payload length."
                    return read_and_byte_swap<std::uint16_t> (reader, io);
                }
                // "If 127, the following 8 bytes interpreted as a 64-bit unsigned integer (the most
                // significant bit MUST be 0) are the payload length. Multibyte length quantities
                // are expressed in network byte order."
                return read_and_byte_swap<std::uint64_t> (reader, io);
            }

        } // end namespace details



        template <typename Reader, typename IO>
        error_or_n<IO, frame> read_frame (Reader & reader, IO io) {
            frame_fixed_layout res{};

            return reader.get_span (
                       io, gsl::make_span (
                               &res, 1)) >>= [&reader](IO io1,
                                                       gsl::span<frame_fixed_layout> const & p1) {
                using return_type = error_or_n<IO, frame>;

                if (p1.size () != 1) {
                    return return_type{ws_error::insufficient_data};
                }

                frame_fixed_layout & part1 = p1.at (0);
                part1.raw = network_to_host (part1.raw);

                if (log_frame_info) {
                    log (pstore::logging::priority::info, "fin: ", part1.fin);
                    log (pstore::logging::priority::info, "rsv: ", part1.rsv);
                    log (pstore::logging::priority::info,
                         "opcode: ", opcode_name (static_cast<opcode> (part1.opcode.value ())));
                    log (pstore::logging::priority::info, "mask: ", part1.mask);
                    log (pstore::logging::priority::info, "payload_length: ", part1.payload_length);
                }

                // "The rsv[n] fields MUST be 0 unless an extension is negotiated that defines
                // meanings for non-zero values. If a nonzero value is received and none of the
                // negotiated extensions defines the meaning of such a nonzero value, the receiving
                // endpoint MUST _Fail the WebSocket Connection_."
                if (part1.rsv != 0) {
                    return return_type{ws_error::reserved_bit_set};
                }

                // "The server MUST close the connection upon receiving a frame that is not masked."
                if (!part1.mask) {
                    return return_type{ws_error::unmasked_frame};
                }

                return details::read_payload_length (
                           reader, io1,
                           part1.payload_length) >>= [&](IO io2, std::uint64_t payload_length) {
                    log (pstore::logging::priority::info, "Payload length: ", payload_length);
                    if ((payload_length & (std::uint64_t{1} << 63U)) != 0U) {
                        // "The most significant bit MUST be 0."
                        return return_type{ws_error::payload_too_long};
                    }

                    constexpr auto mask_length = 4U;
                    std::array<std::uint8_t, mask_length> mask_arr{{0}};
                    return reader.get_span (io2, gsl::make_span (mask_arr)) >>=
                           [&](IO io3, gsl::span<std::uint8_t> const & mask) {
                               if (mask.size () != mask_length) {
                                   return return_type{ws_error::insufficient_data};
                               }

                               std::vector<std::uint8_t> payload (
                                   std::vector<std::uint8_t>::size_type{payload_length},
                                   std::uint8_t{0});

                               return reader.get_span (io3, gsl::make_span (payload)) >>=
                                      [&payload, payload_length, &mask, &part1](
                                          IO io4, gsl::span<std::uint8_t> const & payload_span) {
                                          auto const payload_size = payload_span.size ();
                                          if (payload_size < 0 ||
                                              static_cast<std::make_unsigned<std::remove_const<
                                                      decltype (payload_size)>::type>::type> (
                                                  payload_size) != payload_length) {
                                              return return_type{ws_error::insufficient_data};
                                          }

                                          for (auto ctr = gsl::span<std::uint8_t>::index_type{0};
                                               ctr < payload_size; ++ctr) {
                                              payload_span[ctr] ^= mask[ctr % 4];
                                          }

                                          return return_type{
                                              in_place, io4,
                                              frame{part1.opcode, part1.fin, std::move (payload)}};
                                      };
                           };
                };
            };
        }


        namespace details {

            template <typename LengthType, typename Sender, typename IO>
            error_or<IO> send_extended_length_message (Sender sender, IO io,
                                                       frame_fixed_layout const & f,
                                                       gsl::span<std::uint8_t const> const & span) {
                auto send_length = [&](IO io2) {
                    auto const size = span.size ();
                    assert (
                        size >= 0 &&
                        static_cast<
                            std::make_unsigned<std::remove_const<decltype (size)>::type>::type> (
                            size) <= std::numeric_limits<LengthType>::max ());
                    return send (sender, io2, static_cast<LengthType> (size));
                };
                auto send_payload = [&](IO io3) { return send (sender, io3, span); };

                return (send (sender, io, f.raw) >>= send_length) >>= send_payload;
            }

        } // end namespace details


        // send_message
        // ~~~~~~~~~~~~
        template <typename Sender, typename IO>
        error_or<IO> send_message (Sender sender, IO io, opcode op,
                                   gsl::span<std::uint8_t const> const & span) {
            frame_fixed_layout f{};
            f.fin = true;
            f.rsv = std::uint16_t{0};
            f.opcode = static_cast<std::uint16_t> (op);
            f.mask = false;

            auto const length = span.size ();
            if (length < 126) {
                f.payload_length = static_cast<std::uint16_t> (length);
                return send (sender, io, f.raw) >>=
                       [sender, &span](IO io2) { return send (sender, io2, span); };
            }

            if (length <= std::numeric_limits<std::uint16_t>::max ()) {
                // Length is sent as 16-bits.
                f.payload_length = std::uint16_t{126};
                return details::send_extended_length_message<std::uint16_t> (sender, io, f, span);
            }

            // The payload length must not have the top bit set.
            if (static_cast<std::uint64_t> (length) & (std::uint64_t{1} << 63U)) {
                return error_or<IO>{make_error_code (ws_error::message_too_long)};
            }

            // Send the length as a full 64-bit value.
            f.payload_length = std::uint16_t{127};
            return details::send_extended_length_message<std::uint64_t> (sender, io, f, span);
        }


        // pong
        // ~~~~
        template <typename Sender, typename IO>
        error_or<IO> pong (Sender sender, IO io, gsl::span<std::uint8_t const> const & payload) {
            log (logging::priority::info, "Sending pong. Length=", payload.size ());
            return send_message (sender, io, opcode::pong, payload);
        }


        // send_close_frame
        // ~~~~~~~~~~~~~~~~
        template <typename Sender, typename IO>
        error_or<IO> send_close_frame (Sender sender, IO io, close_status_code status) {
            log (logging::priority::info,
                 "Sending close frame code=", static_cast<std::uint16_t> (status));
            PSTORE_STATIC_ASSERT ((
                std::is_same<std::underlying_type<close_status_code>::type, std::uint16_t>::value));
            std::uint16_t const ns = host_to_network (static_cast<std::uint16_t> (status));
            return send_message (sender, io, opcode::close, as_bytes (gsl::make_span (&ns, 1)));
        }


        // is_valid_utf8
        // ~~~~~~~~~~~~~
        template <typename Iterator>
        bool is_valid_utf8 (Iterator first, Iterator last) {
            utf::utf8_decoder decoder;
            std::for_each (first, last, [&decoder](std::uint8_t cu) { decoder.get (cu); });
            return decoder.is_well_formed ();
        }


        // close_message
        // ~~~~~~~~~~~~~
        template <typename Sender, typename IO>
        error_or<IO> close_message (Sender sender, IO io, frame const & wsp) {
            auto state = close_status_code::normal;
            auto payload_size = wsp.payload.size ();

            // "If there is a body, the first two bytes of the body MUST be a 2-byte unsigned
            // integer (in network byte order) representing a status code with value /code/ defined
            // in Section 7.4. Following the 2-byte integer, the body MAY contain UTF-8-encoded data
            // with value /reason/, the interpretation of which is not defined by this
            // specification."
            if (payload_size == 0U) {
                // That's fine. Just a normal close.
                state = close_status_code::normal;
            } else if (payload_size < sizeof (std::uint16_t)) {
                // Bad message. Payload must be 0 or at least a 2 byte close-code.
                state = close_status_code::protocol_error;
            } else {
                // Extract the close state from the message payload.
                assert (payload_size >= sizeof (std::uint16_t));
                state = static_cast<close_status_code> (network_to_host (
                    *reinterpret_cast<std::uint16_t const *> (wsp.payload.data ())));

                if (!is_valid_close_status_code (static_cast<std::uint16_t> (state))) {
                    state = close_status_code::protocol_error;
                } else if (payload_size > sizeof (std::uint16_t)) {
                    auto first = std::begin (wsp.payload);
                    std::advance (first, sizeof (std::uint16_t));
                    if (!is_valid_utf8 (first, std::end (wsp.payload))) {
                        state = close_status_code::invalid_payload;
                    }
                }
            }

            return send_close_frame (sender, io, state);
        }

        template <typename Sender, typename IO>
        bool check_message_complete (Sender sender, IO io, frame const & wsp, opcode & op,
                                     std::vector<std::uint8_t> & payload) {
            if (wsp.fin) {
                // We've got the complete message. If this was a text message, we need to
                // validate the UTF-8 that it contains.
                //
                // "When an endpoint is to interpret a byte stream as UTF-8 but finds that
                // the byte stream is not, in fact, a valid UTF-8 stream, that endpoint MUST
                // _Fail the WebSocket Connection_.
                if (op == opcode::text &&
                    !is_valid_utf8 (std::begin (payload), std::end (payload))) {

                    send_close_frame (sender, io, close_status_code::invalid_payload);
                    return false;
                }

                if (log_received_messages) {
                    std::string str;
                    std::transform (std::begin (payload), std::end (payload),
                                    std::back_inserter (str),
                                    [](std::uint8_t v) { return static_cast<char> (v); });
                    log (logging::priority::info, "Received: ", str);
                }

                // This implements a simple echo server ATM.
                error_or<IO> const eo3 = send_message (sender, io, op, gsl::make_span (payload));
                if (!eo3) {
                    log (logging::priority::error, "Send error: ", eo3.get_error ().message ());
                }

                payload.clear ();
                op = opcode::unknown;
            }
            return true;
        }


        struct ws_command {
            opcode op = opcode::unknown;
            std::vector<std::uint8_t> payload;
        };

        template <typename Reader, typename Sender, typename IO>
        std::tuple<IO, bool> socket_read (Reader && reader, Sender && sender, IO io,
                                          gsl::not_null<ws_command *> command) {
            error_or_n<IO, frame> const eo = read_frame (reader, io);
            if (!eo) {
                log (logging::priority::error, "Error: ", eo.get_error ().message ());
                auto const error = eo.get_error ();
                if (error == make_error_code (ws_error::unmasked_frame)) {
                    // "The server MUST close the connection upon receiving a frame that is not
                    // masked.  In this case, a server MAY send a Close frame with a status code
                    // of 1002 (protocol error)."
                    send_close_frame (sender, io, close_status_code::protocol_error);
                } else if (error == make_error_code (ws_error::reserved_bit_set)) {
                    send_close_frame (sender, io, close_status_code::protocol_error);
                } else {
                    send_close_frame (sender, io, close_status_code::abnormal_closure);
                }
                return std::tuple<IO, bool>{std::move (io), true};
            }

            io = std::get<0> (eo);
            frame const & wsp = std::get<1> (eo);

            // "All control frames MUST have a payload length of 125 bytes or less and MUST
            // NOT be fragmented."
            if (is_control_frame_opcode (wsp.op) && (!wsp.fin || wsp.payload.size () > 125)) {
                send_close_frame (sender, io, close_status_code::protocol_error);
                return std::tuple<IO, bool>{std::move (io), true};
            }

            switch (wsp.op) {
            case opcode::continuation:
                if (is_control_frame_opcode (command->op)) {
                    send_close_frame (sender, io, close_status_code::protocol_error);
                    return std::tuple<IO, bool>{std::move (io), true};
                }
                command->payload.insert (std::end (command->payload), std::begin (wsp.payload),
                                         std::end (wsp.payload));
                if (!check_message_complete (sender, io, wsp, command->op, command->payload)) {
                    return std::tuple<IO, bool>{std::move (io), true};
                }
                break;

            // Data frame opcodes.
            case opcode::text:
            case opcode::binary:
                // We didn't see a FIN frame before a new data frame.
                if (command->op != opcode::unknown || !command->payload.empty ()) {
                    send_close_frame (sender, io, close_status_code::protocol_error);
                    return std::tuple<IO, bool>{std::move (io), true};
                }

                command->payload = std::move (wsp.payload);
                command->op = wsp.op;
                if (!check_message_complete (sender, io, wsp, command->op, command->payload)) {
                    return std::tuple<IO, bool>{std::move (io), true};
                }
                break;

            case opcode::reserved_nc_1:
            case opcode::reserved_nc_2:
            case opcode::reserved_nc_3:
            case opcode::reserved_nc_4:
            case opcode::reserved_nc_5:
            case opcode::reserved_control_1:
            case opcode::reserved_control_2:
            case opcode::reserved_control_3:
            case opcode::reserved_control_4:
            case opcode::reserved_control_5:
                send_close_frame (sender, io, close_status_code::protocol_error);
                return std::tuple<IO, bool>{std::move (io), true};

            case opcode::close:
                close_message (sender, io, wsp);
                return std::tuple<IO, bool>{std::move (io), true};

            case opcode::ping: pong (sender, io, gsl::make_span (wsp.payload)); break;

            case opcode::pong:
                // TODO: A reply to a ping that we sent.
                break;

            case opcode::unknown: assert (0); break;
            }
            return std::tuple<IO, bool>{std::move (io), false};
        }


        using channel_container_entry =
            std::tuple<gsl::not_null<channel<descriptor_condition_variable> *>,
                       gsl::not_null<descriptor_condition_variable *>>;
        using channel_container = std::unordered_map<std::string, channel_container_entry>;

        // ws_server_loop
        // ~~~~~~~~~~~~~~
        template <typename Reader, typename Sender, typename IO>
        void ws_server_loop (Reader && reader, Sender && sender, IO io, std::string const & uri,
                             channel_container const & channels) {

            ws_command command;
            channel<descriptor_condition_variable>::subscriber_pointer subscription;
            descriptor_condition_variable * cv = nullptr;

            if (uri.length () > 0 && uri[0] == '/') {
                std::string const name = uri.substr (1);
                auto const pos = channels.find (name);
                if (pos != channels.end ()) {
                    subscription = std::get<0> (pos->second)->new_subscriber ();
                    cv = std::get<1> (pos->second);
                } else {
                    log (logging::priority::error, "No channel named: ", name);
                }
            }

            bool done = false;
            while (!done) {
                inputs_ready const avail =
                    block_for_input (reader, io, cv != nullptr ? &cv->wait_descriptor () : nullptr);
                if (avail.socket) {
                    std::tie (io, done) = socket_read (reader, sender, io, &command);
                }
                if (avail.cv) {
                    // There's a message to push to our peer.
                    assert (cv != nullptr);
                    cv->reset ();
                    if (subscription) {
                        while (maybe<std::string> const message = subscription->pop ()) {
                            log (logging::priority::info, "sending:", *message);
                            error_or<IO> const eo3 = send_message (
                                sender, io, opcode::text, as_bytes (gsl::make_span (*message)));
                            if (!eo3) {
                                log (logging::priority::error,
                                     "Send error: ", eo3.get_error ().message ());
                            }
                        }
                    }
                }
            }
        }

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTP_WS_SERVER_HPP
