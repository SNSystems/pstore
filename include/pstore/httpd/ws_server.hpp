#ifndef PSTORE_HTTPD_WS_SERVER_HPP
#define PSTORE_HTTPD_WS_SERVER_HPP

#ifdef _WIN32
#    include <Winsock2.h>
#else
#    include <arpa/inet.h>
#endif //_WIN32
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

        using byte_span = gsl::span<std::uint8_t>;

        // TODO: repeatedly calling reader.geto() is hideously inefficient. Add a span-based method
        // to Reader.
        template <typename Reader, typename IO>
        error_or_n<IO, byte_span> get_span_impl (Reader & reader, IO io, byte_span const & sp) {
            if (sp.size () == 0) {
                return error_or_n<IO, byte_span>{in_place, io, sp};
            }
            return reader.geto (io) >>= [&reader, &sp](IO io1, maybe<std::uint8_t> const & b) {
                if (!b) {
                    return error_or_n<IO, byte_span>{in_place, io1, sp};
                }
                auto data = sp.data ();
                *data = *b;
                return get_span_impl (reader, io1, byte_span{data + 1, sp.size () - 1});
            };
        }

        template <typename Reader, typename IO>
        error_or_n<IO> get_span (Reader & reader, IO io, gsl::span<std::uint8_t> const & sp) {
            return get_span_impl (reader, io, sp) >>=
                   [&sp](IO io2, gsl::span<std::uint8_t> const &) {
                       return error_or_n<IO>{in_place, io2};
                   };
        }

        template <typename Reader, typename IO>
        error_or<std::pair<IO, maybe<packet1>>> get_packet1 (Reader & reader, IO io) {
            packet1 res;

            return get_span (reader, io, gsl::make_span (res.bytes)) >>= [&reader, &res](IO io1) {
                using return_type = error_or<std::pair<IO, maybe<packet1>>>;

                log (pstore::logging::priority::info, "fin:", static_cast<std::uint16_t> (res.fin));
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

                auto payload_length = std::uint64_t{0};

                if (res.payload_length < 126U) {
                    payload_length = res.payload_length;
                } else if (res.payload_length == 126U) {
                    // TODO: read more...
                } else {
                    assert (res.payload_length == 127U);
                    // TODO: read more...
                }
                log (pstore::logging::priority::info, "final payload_length:", payload_length);

                auto mask = std::uint32_t{0};
                if (res.mask) {
                    auto mspan = as_writeable_bytes (gsl::make_span (&mask, 1));
                    get_span (reader, io1, mspan) >>= [&mask](IO io2) {
                        mask = ntohl (mask);
                        log (pstore::logging::priority::info, "mask:", mask);
                        return error_or<IO> (io2);
                    };
                }
                return return_type{in_place, io1, just (res)};
            };
        }

        template <typename Reader, typename IO>
        void ws_server_loop (Reader & reader, IO io) {
            get_packet1 (reader, io);
        }

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTPD_WS_SERVER_HPP
