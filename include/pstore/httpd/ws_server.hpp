//*                                               *
//* __      _____   ___  ___ _ ____   _____ _ __  *
//* \ \ /\ / / __| / __|/ _ \ '__\ \ / / _ \ '__| *
//*  \ V  V /\__ \ \__ \  __/ |   \ V /  __/ |    *
//*   \_/\_/ |___/ |___/\___|_|    \_/ \___|_|    *
//*                                               *
//===- include/pstore/httpd/ws_server.hpp ---------------------------------===//
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
#ifndef PSTORE_HTTPD_WS_SERVER_HPP
#define PSTORE_HTTPD_WS_SERVER_HPP

#ifdef _WIN32
#    include <Winsock2.h>
#else
#    include <arpa/inet.h>
#endif //_WIN32

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/httpd/buffered_reader.hpp"
#include "pstore/httpd/endian.hpp"
#include "pstore/support/bit_field.hpp"
#include "pstore/support/logging.hpp"

#define PSTORE_LOG_FRAME_INFO 0 // Define as 1 to log the frame header as it is received.

namespace pstore {
    namespace httpd {

        enum ws_error : int {
            reserved_bit_set = 1,
            payload_too_long,
            unmasked_frame,
            message_too_long,
        };

        // *********************
        // * ws_error_category *
        // *********************
        class ws_error_category : public std::error_category {
        public:
            // The need for this constructor was removed by CWG defect 253 but Clang (prior
            // to 3.9.0) and GCC (before 4.6.4) require its presence.
            ws_error_category () noexcept {} // NOLINT
            char const * name () const noexcept override;
            std::string message (int error) const override;
        };


        std::error_code make_error_code (ws_error e);

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
            bit_field<std::uint16_t, 12, 1> rsv3;
            bit_field<std::uint16_t, 13, 1> rsv2;
            bit_field<std::uint16_t, 14, 1> rsv1;
            bit_field<std::uint16_t, 15, 1> fin;
        };

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

        enum class close_status_code {
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

        struct frame {
            opcode op = opcode::unknown;
            bool fin = false;
            std::vector<std::uint8_t> payload;
        };



        namespace details {

            template <typename T, typename Reader, typename IO>
            pstore::error_or_n<IO, T> read_and_byte_swap (Reader & reader, IO io) {
                auto v = T{0};
                return read_span (reader, io, pstore::gsl::make_span (&v, 1)) >>=
                       [](IO io2, pstore::gsl::span<T> const & l1) {
                           return pstore::error_or_n<IO, T>{pstore::in_place, io2,
                                                            network_to_host (l1.at (0))};
                       };
            }

            template <typename T, typename Reader, typename IO>
            pstore::error_or_n<IO, std::uint64_t> read_extended_payload_length (Reader & reader,
                                                                                IO io) {
                auto length16n = T{0};
                return read_span (reader, io, pstore::gsl::make_span (&length16n, 1)) >>=
                       [](IO io2, pstore::gsl::span<T> const & l1) {
                           return pstore::error_or_n<IO, T>{pstore::in_place, io2,
                                                            network_to_host (l1.at (0))};
                       };
            }

            template <typename Reader, typename IO>
            pstore::error_or_n<IO, std::uint64_t> read_payload_length (Reader & reader, IO io,
                                                                       unsigned base_length) {
                if (base_length < 126U) {
                    // "If 0-125, that is the payload length."
                    return pstore::error_or_n<IO, std::uint64_t>{pstore::in_place, io, base_length};
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


        using byte_span = gsl::span<std::uint8_t>;

        // TODO: repeatedly calling reader.geto() is hideously inefficient. Add a span-based method
        // to Reader.
        template <typename Reader, typename IO>
        error_or_n<IO, byte_span> read_span_impl (Reader & reader, IO io, byte_span const & sp) {
            if (sp.size () == 0) {
                return error_or_n<IO, byte_span>{in_place, io, sp};
            }
            return reader.geto (io) >>= [&reader, &sp](IO io1, maybe<std::uint8_t> const & b) {
                if (!b) {
                    return error_or_n<IO, byte_span>{in_place, io1, sp};
                }
                auto data = sp.data ();
                *data = *b;
                return read_span_impl (reader, io1, byte_span{data + 1, sp.size () - 1});
            };
        }

        // TODO: move read_span and read_span_impl to buffered_reader.
        template <typename Reader, typename IO, typename SpanType>
        auto read_span (Reader & reader, IO io, SpanType const & sp) -> error_or_n<IO, SpanType> {
            return read_span_impl (reader, io, as_writeable_bytes (sp)) >>=
                   [&sp](IO io2, gsl::span<std::uint8_t> const &) {
                       return error_or_n<IO, SpanType>{in_place, io2, sp};
                   };
        }


        template <typename Reader, typename IO>
        error_or_n<IO, frame> read_frame (Reader & reader, IO io) {
            frame_fixed_layout res{};

            return read_span (
                       reader, io,
                       gsl::make_span (&res,
                                       1)) >>= [&reader](IO io1,
                                                         gsl::span<frame_fixed_layout> const & p1) {
                using return_type = error_or_n<IO, frame>;
                frame_fixed_layout & part1 = p1.at (0);
                part1.raw = network_to_host (part1.raw);

#if PSTORE_LOG_FRAME_INFO
                log (pstore::logging::priority::info, "fin: ", part1.fin);
                log (pstore::logging::priority::info, "rsv1: ", part1.rsv1);
                log (pstore::logging::priority::info, "rsv2: ", part1.rsv2);
                log (pstore::logging::priority::info, "rsv3: ", part1.rsv3);
                log (pstore::logging::priority::info, "opcode: ", part1.opcode);
                log (pstore::logging::priority::info, "mask: ", part1.mask);
                log (pstore::logging::priority::info, "payload_length: ", part1.payload_length);
#endif // PSTORE_LOG_FRAME_INFO

                // "The rsv[n] fields MUST be 0 unless an extension is negotiated that defines
                // meanings for non-zero values. If a nonzero value is received and none of the
                // negotiated extensions defines the meaning of such a nonzero value, the receiving
                // endpoint MUST _Fail the WebSocket Connection_."
                if (part1.rsv1 || part1.rsv2 || part1.rsv3) {
                    return return_type{ws_error::reserved_bit_set};
                }

                return details::read_payload_length (
                           reader, io1,
                           part1.payload_length) >>= [&](IO io2, std::uint64_t payload_length) {
                    log (pstore::logging::priority::info, "Payload length: ", payload_length);
                    if ((payload_length & (std::uint64_t{1} << 63U)) != 0U) {
                        // "The most significant bit MUST be 0."
                        return return_type{ws_error::payload_too_long};
                    }

                    constexpr auto mask_length = 4;
                    std::array<std::uint8_t, mask_length> mask{{0}};
                    if (!part1.mask) {
                        // "The server MUST close the connection upon receiving a frame that is
                        // not masked."
                        return return_type{ws_error::unmasked_frame};
                    }
                    return read_span (reader, io2, gsl::make_span (mask)) >>=
                           [&](IO io3, gsl::span<std::uint8_t> const & mask_span) {
                               if (mask_span.size () != mask_length) {
                                   // FIXME: ERRORS! EOF before mask was received.
                               }

                               std::vector<std::uint8_t> payload (
                                   std::vector<std::uint8_t>::size_type{payload_length},
                                   std::uint8_t{0});

                               return read_span (reader, io3, gsl::make_span (payload)) >>=
                                      [&](IO io4, gsl::span<std::uint8_t> const & payload_span) {
                                          auto const payload_size = payload_span.size ();
                                          using usize =
                                              std::make_unsigned<decltype (payload_size)>::type;
                                          if (payload_size < 0 ||
                                              static_cast<usize> (payload_size) != payload_length) {
                                              // FIXME: ERRORS! EOF before payload was received.
                                          }

                                          for (auto ctr = gsl::span<std::uint8_t>::index_type{0};
                                               ctr < payload_size; ++ctr) {
                                              payload_span[ctr] ^= mask_span[ctr % 4];
                                          }

                                          frame resl;
                                          resl.fin = part1.fin;
                                          resl.op = static_cast<opcode> (
                                              static_cast<std::uint16_t> (part1.opcode));
                                          resl.payload = std::move (payload);
                                          return return_type{in_place, io4, std::move (resl)};
                                      };
                           };
                };
            };
        }

        template <typename Sender, typename IO>
        error_or<IO> pong (Sender sender, IO io) {
            frame_fixed_layout f{};
            f.fin = true;
            f.rsv1 = false;
            f.rsv2 = false;
            f.rsv3 = false;
            f.opcode = static_cast<std::uint16_t> (opcode::pong);
            f.mask = false;
            f.payload_length = 0;
            return send (sender, io, f.raw);
        }

        namespace details {

            template <typename LengthType, typename Sender, typename IO>
            pstore::error_or<IO> send_extended_length_message (Sender sender, IO io,
                                                               frame_fixed_layout const & f,
                                                               std::string const & message) {

                return send (sender, io, f.raw) >>= [sender, &message](IO io2) {
                    return send (sender, io2, static_cast<LengthType> (message.length ())) >>=
                           [sender, &message](IO io3) {
                               return send (sender, io3, as_bytes (gsl::make_span (message)));
                           };
                };
            }

        } // end namespace details

        template <typename Sender, typename IO>
        error_or<IO> send_message (Sender sender, IO io, std::string const & message) {
            frame_fixed_layout f{};
            f.fin = true;
            f.rsv1 = false;
            f.rsv2 = false;
            f.rsv3 = false;
            f.opcode = static_cast<std::uint16_t> (opcode::text);
            f.mask = false;
            assert (message.length () < 126);
            auto const length = message.length ();
            if (length < 126) {
                f.payload_length = length;
                return send (sender, io, f.raw) >>= [sender, &message](IO io2) {
                   return send (sender, io2, as_bytes (gsl::make_span (message)));
               };
            }

            if (length < std::numeric_limits<std::uint16_t>::max ()) {
                // Length is sent as 16-bits.
                f.payload_length = 126;
                return details::send_extended_length_message<std::uint16_t> (sender, io, f,
                                                                             message);
            }

            // The payload length must not have the top bit set.
            if (length & std::uint64_t{1} << 63U) {
                return error_or<IO>{ws_error::message_too_long};
            }

            // Send the length as a full 64-bit value.
            f.payload_length = 127;
            return details::send_extended_length_message<std::uint64_t> (sender, io, f, message);
        }


        template <typename Reader, typename Sender, typename IO>
        void ws_server_loop (Reader & reader, Sender sender, IO io) {
            opcode op = opcode::unknown;
            std::vector<std::uint8_t> payload;
            bool done = false;
            while (!done) {
                error_or_n<IO, frame> eo = read_frame (reader, io);
                if (!eo) {
                    log (logging::priority::error, "Error: ", eo.get_error ().message ());
                    if (eo.get_error () == make_error_code (ws_error::unmasked_frame)) {
                        // TODO: ERROR: The server MUST close the connection upon receiving a frame
                        // that is not masked.  In this case, a server MAY send a Close frame with a
                        // status code of 1002 (protocol error)
                        return;
                    }
                } else {
                    frame const & wsp = std::get<1> (eo);
                    switch (wsp.op) {
                    case opcode::continuation:
                        if ((static_cast<unsigned> (op) & 0x08) != 0U) {
                            // FIXME: ERROR: continuation of a control frame.
                        }
                        payload.insert (std::end (payload), std::begin (wsp.payload),
                                        std::end (wsp.payload));
                        break;

                    // Data frame opcodes.
                    case opcode::text:
                    case opcode::binary:
                        if (!payload.empty ()) {
                            // FIXME: ERROR: We didn't see a FIN frame before a new data frame.
                            return;
                        }
                        payload = std::move (wsp.payload);
                        op = wsp.op;
                        break;

                    case opcode::reserved_nc_1:
                    case opcode::reserved_nc_2:
                    case opcode::reserved_nc_3:
                    case opcode::reserved_nc_4:
                    case opcode::reserved_nc_5:
                        // FIXME: ERROR: Unknown data frame opcode.
                        break;

                    case opcode::reserved_control_1:
                    case opcode::reserved_control_2:
                    case opcode::reserved_control_3:
                    case opcode::reserved_control_4:
                    case opcode::reserved_control_5:
                        // ERROR: unknown opcode.
                        break;

                    case opcode::close: break;

                    case opcode::ping: {
                        error_or_n<IO> eo2 = pong (sender, std::get<0> (eo));
                        if (!eo2) {
                            // ERROR: send failed
                        }
                    } break;

                    case opcode::pong:
                        // TODO: Bad opcode?
                        break;

                    case opcode::unknown: assert (0); break;
                    }

                    if (wsp.fin) {
                        // We've got the complete message.
                        std::string str;
                        std::transform (std::begin (payload), std::end (payload),
                                        std::back_inserter (str),
                                        [](std::uint8_t v) { return static_cast<char> (v); });
                        log (logging::priority::info, "Received: ", str);

                        error_or<IO> const eo3 = send_message (sender, std::get<0> (eo), str);
                        // FIXME: error handling!

                        payload.clear ();
                        op = opcode::unknown;
                    }
                }
            }
        }

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTPD_WS_SERVER_HPP
