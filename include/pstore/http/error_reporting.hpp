//===- include/pstore/http/error_reporting.hpp ------------*- mode: C++ -*-===//
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
/// \file error_reporting.hpp
/// \brief Reporting HTTP server errors to the client.

#include <algorithm>
#include <sstream>

#include "pstore/http/http_date.hpp"
#include "pstore/http/send.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/os/descriptor.hpp"

namespace pstore {
    namespace http {

        using czstring_pair = std::pair<gsl::czstring, gsl::czstring>;

        enum class http_status_code {
            /// The requester has asked the server to switch protocols and the server has agreed to
            /// do so.
            switching_protocols = 101,
            /// The server cannot or will not process the request due to an apparent client error
            /// (e.g., malformed request syntax, size too large, invalid request message framing, or
            /// deceptive request routing).
            bad_request = 400,
            /// The requested resource could not be found but may be available in the future.
            /// Subsequent requests by the client are permissible.
            not_found = 404,
            /// The client should switch to a different protocol such as TLS/1.3, given in the
            /// Upgrade header field.
            upgrade_required = 426,
            /// A generic error message, given when an unexpected condition was encountered and no
            /// more specific message is suitable.
            internal_server_error = 500,
            /// The server either does not recognize the request method, or it lacks the ability to
            /// fulfil the request.
            not_implemented = 501,
        };

        std::string build_status_line (http_status_code const code, gsl::czstring text);

        template <
            typename InputIterator,
            typename = std::enable_if_t<std::is_same<
                typename std::iterator_traits<InputIterator>::value_type, czstring_pair>::value>>
        std::string build_headers (InputIterator first, InputIterator last) {
            std::ostringstream os;
            std::for_each (first, last, [&] (czstring_pair const & p) {
                os << p.first << ": " << p.second << crlf;
            });
            os << "Server: " << server_name << crlf;
            os << crlf;
            return os.str ();
        }

        template <typename Sender, typename IO>
        error_or<IO> send_error_page (Sender sender, IO io, gsl::czstring const cause,
                                      http_status_code const error_no, gsl::czstring const shortmsg,
                                      gsl::czstring const longmsg) {
            using status_type = std::underlying_type_t<http_status_code>;
            std::ostringstream content;
            content << "<!DOCTYPE html>\n"
                       "<html lang=\"en\">"
                       "<head>\n"
                       "<meta charset=\"utf-8\">\n"
                       "<title>"
                    << server_name
                    << " Error</title>\n"
                       "</head>\n"
                       "<body>\n"
                       "<h1>"
                    << server_name
                    << " Web Server Error</h1>\n"
                       "<p>"
                    << static_cast<status_type> (error_no) << ": " << shortmsg
                    << "</p>"
                       "<p>"
                    << longmsg << ": " << cause
                    << "</p>\n"
                       "<hr>\n"
                       "<em>The "
                    << server_name
                    << " Web server</em>\n"
                       "</body>\n"
                       "</html>\n";
            std::string const & content_str = content.str ();

            auto const now = http_date (std::chrono::system_clock::now ());
            auto const status_line = build_status_line (error_no, "OK");

            auto const content_length = std::to_string (content_str.length ());
            std::array<czstring_pair, 5> const h{{
                {"Content-length", content_length.c_str ()},
                {"Connection", "close"}, // TODO remove this when we support persistent connections
                {"Content-type", "text/html"},
                {"Date", now.c_str ()},
                {"Last-Modified", now.c_str ()},
            }};

            // Send the three parts: the response line, the headers, and the HTML content.
            return send (sender, io, status_line + crlf) >>= [&] (IO io2) {
                return send (sender, io2, build_headers (std::begin (h), std::end (h))) >>=
                       [&] (IO io3) { return send (sender, io3, content_str); };
            };
        }


        class request_info;
        /// Here we bridge from the std::error_code world to HTTP status codes.
        void report_error (std::error_code const error, request_info const & request,
                           socket_descriptor & socket);

    } // end namespace http
} // end namespace pstore
