//===- lib/http/error_reporting.cpp ---------------------------------------===//
//*                                                       _   _              *
//*   ___ _ __ _ __ ___  _ __   _ __ ___ _ __   ___  _ __| |_(_)_ __   __ _  *
//*  / _ \ '__| '__/ _ \| '__| | '__/ _ \ '_ \ / _ \| '__| __| | '_ \ / _` | *
//* |  __/ |  | | | (_) | |    | | |  __/ |_) | (_) | |  | |_| | | | | (_| | *
//*  \___|_|  |_|  \___/|_|    |_|  \___| .__/ \___/|_|   \__|_|_| |_|\__, | *
//*                                     |_|                           |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file error_reporting.cpp
/// \brief Reporting HTTP server errors to the client.

#include "pstore/http/error_reporting.hpp"

#include "pstore/http/error.hpp"
#include "pstore/http/net_txrx.hpp"
#include "pstore/http/request.hpp"
#include "pstore/http/ws_server.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/romfs/romfs.hpp"

namespace {
    // send bad websocket version
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~
    // From RFC 6455: "The |Sec-WebSocket-Version| header field in the client's handshake
    // includes the version of the WebSocket Protocol with which the client is
    // attempting to communicate. If this version does not match a version understood by
    // the server, the server MUST abort the WebSocket handshake described in this
    // section and instead send an appropriate HTTP error code (such as 426 Upgrade
    // Required) and a |Sec-WebSocket-Version| header field indicating the version(s)
    // the server is capable of understanding."
    template <typename SenderFunction, typename IO>
    pstore::error_or<IO> send_bad_websocket_version (SenderFunction const sender, IO io) {
        std::string const response_string =
            build_status_line (pstore::http::http_status_code::upgrade_required, "OK");

        return sender (io, as_bytes (pstore::gsl::make_span (response_string))) >>=
               [&sender] (IO io2) {
                   auto const ws_version = std::to_string (pstore::http::ws_version);
                   std::array<pstore::http::czstring_pair, 1> const headers{{
                       {"Sec-WebSocket-Version", ws_version.c_str ()},
                   }};
                   std::string const h =
                       pstore::http::build_headers (std::begin (headers), std::end (headers));
                   return sender (io2, as_bytes (pstore::gsl::make_span (h)));
               };
    }

} // end anonymous namespace

namespace pstore {
    namespace http {

        std::string build_status_line (http_status_code const code, gsl::czstring text) {
            std::ostringstream str;
            str << "HTTP/1.1 " << static_cast<std::underlying_type_t<http_status_code>> (code)
                << ' ' << text << http::crlf;
            return str.str ();
        }

        // Here we bridge from the std::error_code world to HTTP status codes.
        void report_error (std::error_code const error, request_info const & request,
                           socket_descriptor & socket) {
            static constexpr auto sender = net::network_sender;

            auto const report = [&] (http_status_code const code, gsl::czstring const message) {
                send_error_page (sender, std::ref (socket), request.uri ().c_str (), code, message,
                                 error.message ().c_str ());
            };

            log (logger::priority::error, "http error: ", error.message ());
            auto const & cat = error.category ();
            if (cat == get_error_category ()) {
                switch (static_cast<error_code> (error.value ())) {
                case error_code::bad_request:
                    return report (http_status_code::bad_request, "Bad request");

                case error_code::bad_websocket_version:
                    send_bad_websocket_version (sender, std::ref (socket));
                    return;

                case error_code::not_implemented:
                    return report (http_status_code::not_implemented, "Not implemented");

                case error_code::string_too_long:
                case error_code::refill_out_of_range: break;
                }
            } else if (cat == romfs::get_romfs_error_category ()) {
                switch (static_cast<romfs::error_code> (error.value ())) {
                case romfs::error_code::enoent:
                case romfs::error_code::enotdir:
                    return report (http_status_code::not_found, "Not found");

                case romfs::error_code::einval: break;
                }
            }

            // Some error that we don't know how to report properly.
            std::ostringstream message;
            message << "Server internal error: " << error.message ();
            report (http_status_code::internal_server_error, message.str ().c_str ());
        }

    } // end namespace http
} // end namespace pstore
