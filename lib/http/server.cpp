//*                                *
//*  ___  ___ _ ____   _____ _ __  *
//* / __|/ _ \ '__\ \ / / _ \ '__| *
//* \__ \  __/ |   \ V /  __/ |    *
//* |___/\___|_|    \_/ \___|_|    *
//*                                *
//===- lib/http/server.cpp ------------------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
#include "pstore/http/server.hpp"

// Standard library includes
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

// OS-specific includes
#ifdef _WIN32

#    include <io.h>
#    include <winsock2.h>
#    include <ws2tcpip.h>

#else

#    include <netdb.h>
#    include <sys/socket.h>
#    include <sys/types.h>

#endif

// Local includes
#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/http/buffered_reader.hpp"
#include "pstore/http/error.hpp"
#include "pstore/http/headers.hpp"
#include "pstore/http/net_txrx.hpp"
#include "pstore/http/query_to_kvp.hpp"
#include "pstore/http/quit.hpp"
#include "pstore/http/request.hpp"
#include "pstore/http/send.hpp"
#include "pstore/http/serve_dynamic_content.hpp"
#include "pstore/http/serve_static_content.hpp"
#include "pstore/http/server_status.hpp"
#include "pstore/http/ws_server.hpp"
#include "pstore/http/wskey.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/support/pubsub.hpp"

namespace {

    using socket_descriptor = pstore::broker::socket_descriptor;

    template <typename Sender, typename IO>
    pstore::error_or<IO> cerror (Sender sender, IO io, pstore::gsl::czstring const cause,
                                 unsigned const error_no, pstore::gsl::czstring const shortmsg,
                                 pstore::gsl::czstring const longmsg) {
        static auto const & crlf = pstore::httpd::crlf;

        std::ostringstream content;
        content << "<!DOCTYPE html>\n"
                   "<html lang=\"en\">"
                   "<head>\n"
                   "<meta charset=\"utf-8\">\n"
                   "<title>pstore-httpd Error</title>\n"
                   "</head>\n"
                   "<body>\n"
                   "<h1>pstore-httpd Web Server Error</h1>\n"
                   "<p>"
                << error_no << ": " << shortmsg
                << "</p>"
                   "<p>"
                << longmsg << ": " << cause
                << "</p>\n"
                   "<hr>\n"
                   "<em>The pstore-httpd Web server</em>\n"
                   "</body>\n"
                   "</html>\n";
        std::string const & content_str = content.str ();
        auto const now = std::chrono::system_clock::now ();

        std::ostringstream os;
        // clang-format off
        os << "HTTP/1.1 " << error_no << " OK" << crlf
           << "Server: pstore-httpd" << crlf
           << "Content-length: " << content_str.length () << crlf
           << "Connection: close" << crlf  // TODO remove this when we support persistent connections
           << "Content-type: " << "text/html" << crlf
           << "Date: " << pstore::httpd::http_date (now) << crlf
           << "Last-Modified: " << pstore::httpd::http_date (now) << crlf
           << crlf
           << content_str;
        // clang-format on
        return pstore::httpd::send (sender, io, os);
    }

    // initialize_socket
    // ~~~~~~~~~~~~~~~~~
    pstore::error_or<socket_descriptor> initialize_socket (in_port_t const port_number) {
        using eo = pstore::error_or<socket_descriptor>;

        log (pstore::logging::priority::info, "initializing on port: ", port_number);
        socket_descriptor fd{::socket (AF_INET, SOCK_STREAM, 0)};
        if (!fd.valid ()) {
            return eo{pstore::httpd::get_last_error ()};
        }

        int const optval = 1;
        if (::setsockopt (fd.native_handle (), SOL_SOCKET, SO_REUSEADDR,
                          reinterpret_cast<char const *> (&optval), sizeof (optval))) {
            return eo{pstore::httpd::get_last_error ()};
        }

        sockaddr_in server_addr{}; // server's addr.
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl (INADDR_ANY); // NOLINT
        server_addr.sin_port = htons (port_number);       // NOLINT
        if (::bind (fd.native_handle (), reinterpret_cast<sockaddr *> (&server_addr),
                    sizeof (server_addr)) < 0) {
            return eo{pstore::httpd::get_last_error ()};
        }

        // Get ready to accept connection requests.
        if (::listen (fd.native_handle (), 5) < 0) { // allow 5 requests to queue up.
            return eo{pstore::httpd::get_last_error ()};
        }

        return eo{std::move (fd)};
    }


    pstore::error_or<std::string> get_client_name (sockaddr_in const & client_addr) {
        std::array<char, 64> host_name{{'\0'}};
        constexpr std::size_t size = host_name.size ();
#ifdef _WIN32
        using size_type = DWORD;
#else
        using size_type = std::size_t;
#endif //!_WIN32
        int const gni_err = ::getnameinfo (
            reinterpret_cast<sockaddr const *> (&client_addr), sizeof (client_addr),
            host_name.data (), static_cast<size_type> (size), nullptr, socklen_t{0}, 0 /*flags*/);
        if (gni_err != 0) {
            return pstore::error_or<std::string>{pstore::httpd::get_last_error ()};
        }
        host_name.back () = '\0'; // guarantee nul termination.
        return pstore::error_or<std::string>{std::string{host_name.data ()}};
    }


    // Here we bridge from the std::error_code world to HTTP status codes.
    void report_error (std::error_code error, pstore::httpd::request_info const & request,
                       socket_descriptor & socket) {
        static constexpr auto crlf = pstore::httpd::crlf;
        static constexpr auto sender = pstore::httpd::net::network_sender;

        auto report = [&error, &request, &socket](unsigned const code,
                                                  pstore::gsl::czstring const message) {
            cerror (sender, std::ref (socket), request.uri ().c_str (), code, message,
                    error.message ().c_str ());
        };

        log (pstore::logging::priority::error, "Error:", error.message ());
        if (error.category () == pstore::httpd::get_error_category ()) {
            switch (static_cast<pstore::httpd::error_code> (error.value ())) {
            case pstore::httpd::error_code::bad_request: report (400, "Bad request"); return;

            case pstore::httpd::error_code::bad_websocket_version: {
                // From rfc6455: "The |Sec-WebSocket-Version| header field in the client's handshake
                // includes the version of the WebSocket Protocol with which the client is
                // attempting to communicate. If this version does not match a version understood by
                // the server, the server MUST abort the WebSocket handshake described in this
                // section and instead send an appropriate HTTP error code (such as 426 Upgrade
                // Required) and a |Sec-WebSocket-Version| header field indicating the version(s)
                // the server is capable of understanding."
                std::ostringstream os;
                os << "HTTP/1.1 426 OK" << crlf //
                   << "Sec-WebSocket-Version: " << pstore::httpd::ws_version << crlf
                   << "Server: pstore-httpd" << crlf //
                   << crlf;
                std::string const & reply = os.str ();
                sender (socket, as_bytes (pstore::gsl::make_span (reply)));
            }
                return;

            case pstore::httpd::error_code::not_implemented:
            case pstore::httpd::error_code::string_too_long:
            case pstore::httpd::error_code::refill_out_of_range: break;
            }
        } else if (error.category () == pstore::romfs::get_romfs_error_category ()) {
            switch (static_cast<pstore::romfs::error_code> (error.value ())) {
            case pstore::romfs::error_code::enoent:
            case pstore::romfs::error_code::enotdir: report (404, "Not found"); return;

            case pstore::romfs::error_code::einval: break;
            }
        }

        // Some error that we don't know how to report properly.
        std::ostringstream message;
        message << "Server internal error: " << error.message ();
        report (501, message.str ().c_str ());
    }


    template <typename Reader, typename IO>
    pstore::error_or<std::unique_ptr<std::thread>>
    upgrade_to_ws (Reader & reader, IO io, pstore::httpd::request_info const & request,
                   pstore::httpd::header_info const & header_contents,
                   pstore::httpd::channel_container const & channels) {
        using return_type = pstore::error_or<std::unique_ptr<std::thread>>;
        using pstore::logging::priority;
        assert (header_contents.connection_upgrade && header_contents.upgrade_to_websocket);

        log (priority::info, "WebSocket upgrade requested");
        // Validate the request headers
        if (!header_contents.websocket_key || !header_contents.websocket_version) {
            log (priority::error, "Missing WebSockets upgrade key or version header.");
            return return_type{pstore::httpd::error_code::bad_request};
        }


        if (*header_contents.websocket_version != pstore::httpd::ws_version) {
            log (priority::error, "Bad Websocket version number requested");
            return return_type{pstore::httpd::error_code::bad_websocket_version};
        }


        // Send back the server handshake response.
        auto const accept_ws_connection = [&header_contents, &io]() {
            log (priority::info, "Accepting WebSockets upgrade");

            static constexpr auto crlf = pstore::httpd::crlf;
            std::string const date = pstore::httpd::http_date (std::chrono::system_clock::now ());
            std::ostringstream os;
            // clang-format off
            os << "HTTP/1.1 101 Switching Protocols" << crlf
               << "Server: pstore-httpd" << crlf
               << "Upgrade: WebSocket" << crlf
               << "Connection: Upgrade" << crlf
               << "Sec-WebSocket-Accept: " << pstore::httpd::source_key (*header_contents.websocket_key) << crlf
               << "Date: " << date << crlf
               << "Last-Modified: " << date << crlf
               << crlf;
            // clang-format on

            // Here I assume that the send() IO param is the same as the Reader's IO parameter.
            return pstore::httpd::send (pstore::httpd::net::network_sender, std::ref (io), os);
        };

        auto server_loop_thread = [&channels](Reader && reader2, socket_descriptor io2,
                                              std::string const uri) {
            PSTORE_TRY {
                constexpr auto ident = "websocket";
                pstore::threads::set_name (ident);
                pstore::logging::create_log_stream (ident);

                log (priority::info, "Started WebSockets session");

                assert (io2.valid ());
                ws_server_loop (std::move (reader2), pstore::httpd::net::network_sender,
                                std::ref (io2), uri, channels);

                log (priority::info, "Ended WebSockets session");
            }
            // clang-format off
            PSTORE_CATCH (std::exception const & ex, { //clang-format on
                log (priority::error, "Error: ", ex.what ());
            })
            // clang-format off
            PSTORE_CATCH (..., { // clang-format on
                log (priority::error, "Unknown exception");
            })
        };

        // Spawn a thread to manage this WebSockets session.
        auto const create_ws_server = [&reader, server_loop_thread,
                                       &request](socket_descriptor & s) {
            assert (s.valid ());
            return return_type{pstore::in_place,
                               new std::thread (server_loop_thread, std::move (reader),
                                                std::move (s), request.uri ())};
        };

        assert (io.get ().valid ());
        return accept_ws_connection () >>= create_ws_server;
    }

#ifndef NDEBUG
    template <typename BufferedReader>
    bool input_is_empty (BufferedReader const & reader, socket_descriptor const & fd) {
        if (reader.available () == 0) {
            return true;
        }
#    ifdef _WIN32
        // TODO: not yet implemented for Windows.
        (void) fd;
        return true;
#    else
        // There should be nothing in the buffered-reader or the input socket waiting to be read.
        errno = 0;
        std::array<std::uint8_t, 256> buf;
        ssize_t const nread = recv (fd.native_handle (), buf.data (),
                                    static_cast<int> (buf.size ()), MSG_PEEK /*flags*/);
        int const err = errno;
        if (nread == -1) {
            log (pstore::logging::priority::error, "error:", err);
        }
        return nread == 0 || nread == -1;
#    endif
    }
#endif

} // end anonymous namespace

namespace pstore {
    namespace httpd {

        int server (romfs::romfs & file_system, gsl::not_null<server_status *> const status,
                    channel_container const & channels) {
            pstore::error_or<socket_descriptor> eparentfd = initialize_socket (status->port ());
            if (!eparentfd) {
                log (logging::priority::error, "opening socket", eparentfd.get_error ().message ());
                return 0;
            }
            socket_descriptor const & parentfd = eparentfd.get ();

            log (logging::priority::info, "starting server-loop");

            std::vector<std::unique_ptr<std::thread>> websockets_workers;

            for (auto expected_state = server_status::http_state::initializing;
                 status->listening (expected_state);
                 expected_state = server_status::http_state::listening) {

                // Wait for a connection request.
                sockaddr_in client_addr{}; // client address.
                auto clientlen = static_cast<socklen_t> (sizeof (client_addr));
                socket_descriptor childfd{
                    ::accept (parentfd.native_handle (),
                              reinterpret_cast<struct sockaddr *> (&client_addr), &clientlen)};
                if (!childfd.valid ()) {
                    log (logging::priority::error, "accept", get_last_error ().message ());
                    continue;
                }

                // Determine who sent the message.
                assert (clientlen == static_cast<socklen_t> (sizeof (client_addr)));
                pstore::error_or<std::string> ename = get_client_name (client_addr);
                if (!ename) {
                    log (logging::priority::error, "getnameinfo", ename.get_error ().message ());
                    continue;
                }
                log (logging::priority::info, "Connection from ", ename.get ());

                // Get the HTTP request line.
                auto reader = make_buffered_reader<socket_descriptor &> (net::refiller);

                assert (childfd.valid ());
                pstore::error_or_n<socket_descriptor &, request_info> eri =
                    read_request (reader, std::ref (childfd));
                if (!eri) {
                    log (logging::priority::error,
                         "Failed reading HTTP request: ", eri.get_error ().message ());
                    continue;
                }
                childfd = std::move (std::get<0> (eri));
                request_info const & request = std::get<1> (eri);
                log (logging::priority::info, "Request: ",
                     request.method () + ' ' + request.version () + ' ' + request.uri ());

                // We only currently support the GET method.
                if (request.method () != "GET") {
                    cerror (pstore::httpd::net::network_sender, std::ref (childfd),
                            request.method ().c_str (), 501, "Not Implemented",
                            "httpd does not implement this method");
                    continue;
                }

                // Respond appropriately based on the request and headers.
                auto const serve_reply =
                    [&](socket_descriptor & io2,
                        header_info const & header_contents) -> std::error_code {
                    if (header_contents.connection_upgrade &&
                        header_contents.upgrade_to_websocket) {

                        pstore::error_or<std::unique_ptr<std::thread>> p = upgrade_to_ws (
                            reader, std::ref (childfd), request, header_contents, channels);
                        if (p) {
                            websockets_workers.emplace_back (std::move (*p));
                        }
                        return p.get_error ();
                    }

                    if (!details::starts_with (request.uri (), dynamic_path)) {
                        return serve_static_content (pstore::httpd::net::network_sender,
                                                     std::ref (io2), request.uri (), file_system)
                            .get_error ();
                    }

                    return serve_dynamic_content (pstore::httpd::net::network_sender,
                                                  std::ref (io2), request.uri ())
                        .get_error ();
                };

                // Scan the HTTP headers.
                assert (childfd.valid ());
                std::error_code const err = read_headers (
                    reader, std::ref (childfd),
                    [](header_info io, std::string const & key, std::string const & value) {
                        return io.handler (key, value);
                    },
                    header_info ()) >>= serve_reply;

                if (err) {
                    // Report the error to the user as an HTTP error.
                    report_error (err, request, childfd);
                }

                assert (input_is_empty (reader, childfd));
            }

            for (std::unique_ptr<std::thread> const & worker : websockets_workers) {
                worker->join ();
            }

            return 0;
        }

    } // end namespace httpd
} // end namespace pstore
