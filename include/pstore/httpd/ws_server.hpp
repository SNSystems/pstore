#ifndef PSTORE_HTTPD_WS_SERVER_HPP
#define PSTORE_HTTPD_WS_SERVER_HPP

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/httpd/buffered_reader.hpp"
#include "pstore/support/bit_field.hpp"
#include "pstore/support/logging.hpp"

namespace pstore {
    namespace httpd {

        // Frame format:
        //
        //       0                   1                   2                   3
        //       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        //      +-+-+-+-+-------+-+-------------+-------------------------------+
        //      |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
        //      |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
        //      |N|V|V|V|       |S|             |   (if payload len==126/127)   |
        //      | |1|2|3|       |K|             |                               |
        //      +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
        //      |     Extended payload length continued, if payload len == 127  |
        //      + - - - - - - - - - - - - - - - +-------------------------------+
        //      |                               |Masking-key, if MASK set to 1  |
        //      +-------------------------------+-------------------------------+
        //      | Masking-key (continued)       |          Payload Data         |
        //      +-------------------------------- - - - - - - - - - - - - - - - +
        //      :                     Payload Data continued ...                :
        //      + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
        //      |                     Payload Data continued ...                |
        //      +---------------------------------------------------------------+

        union packet1 {
            std::uint8_t bytes[2];
            bit_field<std::uint16_t, 0, 1> fin;
            bit_field<std::uint16_t, 1, 1> rsv1;
            bit_field<std::uint16_t, 2, 1> rsv2;
            bit_field<std::uint16_t, 3, 1> rsv3;
            bit_field<std::uint16_t, 4, 4> opcode;
            bit_field<std::uint16_t, 8, 1> mask;
            bit_field<std::uint16_t, 9, 7> payload_length;
        };

        template <typename Reader, typename IO>
        error_or<std::pair<IO, maybe<packet1>>> get_packet1 (Reader & reader, IO io) {
            using rtype = std::pair<IO, maybe<std::uint8_t>>;
            return reader.geto (io) >>= [&reader](rtype const & p1) {
                return reader.geto (std::get<0> (p1)) >>= [&p1](rtype const & p2) {
                    using return_type = error_or<std::pair<IO, maybe<packet1>>>;

                    auto byte1 = std::get<1> (p1);
                    auto byte2 = std::get<1> (p2);
                    if (!byte1 || !byte2) {
                        return return_type{in_place, std::get<0> (p2), nothing<packet1> ()};
                    }

                    packet1 res;
                    res.bytes[0] = *byte1;
                    res.bytes[1] = *byte2;

                    log (pstore::logging::priority::info,
                         "fin:", static_cast<std::uint16_t> (res.fin));
                    log (pstore::logging::priority::info,
                         "rsv1:", static_cast<std::uint16_t> (res.rsv1));
                    log (pstore::logging::priority::info,
                         "rsv2:", static_cast<std::uint16_t> (res.rsv2));
                    log (pstore::logging::priority::info,
                         "rsv3:", static_cast<std::uint16_t> (res.rsv3));
                    log (pstore::logging::priority::info,
                         "opcode:", static_cast<std::uint16_t> (res.opcode));
                    log (pstore::logging::priority::info,
                         "mask:", static_cast<std::uint16_t> (res.mask));
                    log (pstore::logging::priority::info,
                         "payload_length:", static_cast<std::uint16_t> (res.payload_length));

                    return return_type{in_place, std::get<0> (p2), just (res)};
                };
            };
        }

        template <typename Reader, typename IO>
        void ws_server_loop (Reader & reader, IO io) {
            get_packet1 (reader, io);
        }

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTPD_WS_SERVER_HPP
