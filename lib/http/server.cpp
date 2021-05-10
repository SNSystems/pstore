//===- lib/http/server.cpp ------------------------------------------------===//
//*                                *
//*  ___  ___ _ ____   _____ _ __  *
//* / __|/ _ \ '__\ \ / / _ \ '__| *
//* \__ \  __/ |   \ V /  __/ |    *
//* |___/\___|_|    \_/ \___|_|    *
//*                                *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file server.cpp
/// \brief Implements the top-level HTTP server functions.
#include "pstore/http/server.hpp"

// Standard library includes
#include <cstring>
#include <thread>

// OS-specific includes
#ifdef _WIN32
#    include <ws2tcpip.h>
#else
#    include <netdb.h>
#    include <sys/socket.h>
#endif

// Local includes
#include "pstore/http/headers.hpp"
#include "pstore/http/net_txrx.hpp"
#include "pstore/http/request.hpp"
#include "pstore/http/serve_dynamic_content.hpp"
#include "pstore/http/serve_static_content.hpp"
#include "pstore/http/wskey.hpp"

using namespace std::literals::string_literals;

namespace {

    using czstring_pair = std::pair<pstore::gsl::czstring, pstore::gsl::czstring>;

    enum class http_status_code {
        /// The requester has asked the server to switch protocols and the server has agreed to do
        /// so.
        switching_protocols = 101,
        /// The server cannot or will not process the request due to an apparent client error (e.g.,
        /// malformed request syntax, size too large, invalid request message framing, or deceptive
        /// request routing).
        bad_request = 400,
        /// The requested resource could not be found but may be available in the future. Subsequent
        /// requests by the client are permissible.
        not_found = 404,
        /// The client should switch to a different protocol such as TLS/1.3, given in the Upgrade
        /// header field.
        upgrade_required = 426,
        /// A generic error message, given when an unexpected condition was encountered and no more
        /// specific message is suitable.
        internal_server_error = 500,
        /// The server either does not recognize the request method, or it lacks the ability to
        /// fulfil the request.
        not_implemented = 501,
    };

    std::string build_status_line (http_status_code const code, pstore::gsl::czstring text) {
        std::ostringstream str;
        str << "HTTP/1.1 " << static_cast<std::underlying_type_t<http_status_code>> (code) << ' '
            << text << pstore::http::crlf;
        return str.str ();
    }

    template <typename InputIterator,
              typename = std::enable_if_t<std::is_same<
                  typename std::iterator_traits<InputIterator>::value_type, czstring_pair>::value>>
    std::string build_headers (InputIterator first, InputIterator last) {
        std::ostringstream os;
        static constexpr auto crlf = pstore::http::crlf;
        std::for_each (first, last, [&] (czstring_pair const & p) {
            os << p.first << ": " << p.second << crlf;
        });
        os << "Server: " << pstore::http::server_name << crlf;
        os << crlf;
        return os.str ();
    }

    template <typename Sender, typename IO>
    pstore::error_or<IO> cerror (Sender sender, IO io, pstore::gsl::czstring const cause,
                                 http_status_code const error_no,
                                 pstore::gsl::czstring const shortmsg,
                                 pstore::gsl::czstring const longmsg) {
        static constexpr auto crlf = pstore::http::crlf;
        using status_type = std::underlying_type_t<http_status_code>;
        std::ostringstream content;
        content << "<!DOCTYPE html>\n"
                   "<html lang=\"en\">"
                   "<head>\n"
                   "<meta charset=\"utf-8\">\n"
                   "<title>"
                << pstore::http::server_name
                << " Error</title>\n"
                   "</head>\n"
                   "<body>\n"
                   "<h1>"
                << pstore::http::server_name
                << " Web Server Error</h1>\n"
                   "<p>"
                << static_cast<status_type> (error_no) << ": " << shortmsg
                << "</p>"
                   "<p>"
                << longmsg << ": " << cause
                << "</p>\n"
                   "<hr>\n"
                   "<em>The "
                << pstore::http::server_name
                << " Web server</em>\n"
                   "</body>\n"
                   "</html>\n";
        std::string const & content_str = content.str ();

        auto const now = pstore::http::http_date (std::chrono::system_clock::now ());
        auto const status_line = build_status_line (error_no, "OK");

        auto const content_length = std::to_string (content_str.length ());
        std::array<czstring_pair, 6> const h{{
            {"Content-length", content_length.c_str ()},
            {"Connection", "close"}, // TODO remove this when we support persistent connections
            {"Content-type", "text/html"},
            {"Date", now.c_str ()},
            {"Last-Modified", now.c_str ()},
        }};

        // Send the three parts: the response line, the headers, and the HTML content.
        return pstore::http::send (sender, io, status_line + crlf) >>= [&] (IO io2) {
            return pstore::http::send (sender, io2,
                                       build_headers (std::begin (h), std::end (h))) >>=
                   [&] (IO io3) { return pstore::http::send (sender, io3, content_str); };
        };
    }

    using socket_descriptor = pstore::socket_descriptor;

    // initialize socket
    // ~~~~~~~~~~~~~~~~~
    pstore::error_or<socket_descriptor> initialize_socket (in_port_t const port_number) {
        using eo = pstore::error_or<socket_descriptor>;

        log (pstore::logger::priority::info, "initializing on port: ", port_number);
        socket_descriptor fd{::socket (AF_INET, SOCK_STREAM, 0)};
        if (!fd.valid ()) {
            return eo{pstore::http::get_last_error ()};
        }

        int const optval = 1;
        if (::setsockopt (fd.native_handle (), SOL_SOCKET, SO_REUSEADDR,
                          reinterpret_cast<char const *> (&optval), sizeof (optval))) {
            return eo{pstore::http::get_last_error ()};
        }

        sockaddr_in server_addr{}; // server's addr.
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl (INADDR_ANY); // NOLINT
        server_addr.sin_port = htons (port_number);       // NOLINT
        if (::bind (fd.native_handle (), reinterpret_cast<sockaddr *> (&server_addr),
                    sizeof (server_addr)) < 0) {
            return eo{pstore::http::get_last_error ()};
        }

        // Get ready to accept connection requests.
        if (::listen (fd.native_handle (), 5) < 0) { // allow 5 requests to queue up.
            return eo{pstore::http::get_last_error ()};
        }

        return eo{std::move (fd)};
    }

    // get client name
    // ~~~~~~~~~~~~~~~
    pstore::error_or<std::string> get_client_name (sockaddr_in const & client_addr) {
        std::array<char, 64> host_name{{'\0'}};
        constexpr std::size_t size = host_name.size ();
#ifdef _WIN32
        using size_type = DWORD;
#else
        using size_type = std::size_t;
#endif //!_WIN32

        if (::getnameinfo (reinterpret_cast<sockaddr const *> (&client_addr), sizeof (client_addr),
                           host_name.data (), static_cast<size_type> (size), nullptr, socklen_t{0},
                           0 /*flags*/) != 0) {
            return pstore::error_or<std::string>{pstore::http::get_last_error ()};
        }
        host_name.back () = '\0'; // guarantee nul termination.
        return pstore::error_or<std::string>{pstore::in_place, host_name.data ()};
    }

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
            build_status_line (http_status_code::upgrade_required, "OK");

        return sender (io, as_bytes (pstore::gsl::make_span (response_string))) >>=
               [&sender] (IO io2) {
                   auto const ws_version = std::to_string (pstore::http::ws_version);
                   std::array<czstring_pair, 2> const headers{{
                       {"Sec-WebSocket-Version", ws_version.c_str ()},
                   }};
                   std::string const h = build_headers (std::begin (headers), std::end (headers));
                   return sender (io2, as_bytes (pstore::gsl::make_span (h)));
               };
    }

    // Here we bridge from the std::error_code world to HTTP status codes.
    void report_error (std::error_code const error, pstore::http::request_info const & request,
                       socket_descriptor & socket) {
        static constexpr auto sender = pstore::http::net::network_sender;

        auto report = [&error, &request, &socket] (http_status_code const code,
                                                   pstore::gsl::czstring const message) {
            cerror (sender, std::ref (socket), request.uri ().c_str (), code, message,
                    error.message ().c_str ());
        };

        log (pstore::logger::priority::error, "http error: ", error.message ());
        auto const & cat = error.category ();
        if (cat == pstore::http::get_error_category ()) {
            switch (static_cast<pstore::http::error_code> (error.value ())) {
            case pstore::http::error_code::bad_request:
                return report (http_status_code::bad_request, "Bad request");

            case pstore::http::error_code::bad_websocket_version:
                send_bad_websocket_version (sender, std::ref (socket));
                return;

            case pstore::http::error_code::not_implemented:
                return report (http_status_code::not_implemented, "Not implemented");

            case pstore::http::error_code::string_too_long:
            case pstore::http::error_code::refill_out_of_range: break;
            }
        } else if (cat == pstore::romfs::get_romfs_error_category ()) {
            switch (static_cast<pstore::romfs::error_code> (error.value ())) {
            case pstore::romfs::error_code::enoent:
            case pstore::romfs::error_code::enotdir:
                return report (http_status_code::not_found, "Not found");

            case pstore::romfs::error_code::einval: break;
            }
        }

        // Some error that we don't know how to report properly.
        std::ostringstream message;
        message << "Server internal error: " << error.message ();
        report (http_status_code::internal_server_error, message.str ().c_str ());
    }


    template <typename Reader, typename IO>
    pstore::error_or<std::unique_ptr<std::thread>>
    upgrade_to_ws (Reader & reader, IO io, pstore::http::request_info const & request,
                   pstore::http::header_info const & header_contents,
                   pstore::http::channel_container const & channels) {
        using return_type = pstore::error_or<std::unique_ptr<std::thread>>;
        using priority = pstore::logger::priority;
        PSTORE_ASSERT (header_contents.connection_upgrade && header_contents.upgrade_to_websocket);

        log (priority::info, "WebSocket upgrade requested");
        // Validate the request headers
        if (!header_contents.websocket_key || !header_contents.websocket_version) {
            log (priority::error, "Missing WebSockets upgrade key or version header.");
            return return_type{pstore::http::error_code::bad_request};
        }


        if (*header_contents.websocket_version != pstore::http::ws_version) {
            log (priority::error, "Bad Websocket version number requested");
            return return_type{pstore::http::error_code::bad_websocket_version};
        }


        // Send back the server handshake response.
        auto const accept_ws_connection = [&header_contents, &io] () {
            log (priority::info, "Accepting WebSockets upgrade");

            std::string const date = pstore::http::http_date (std::chrono::system_clock::now ());
            std::string const accept = pstore::http::source_key (*header_contents.websocket_key);

            std::array<czstring_pair, 5> headers{{
                {"Upgrade", "WebSocket"},
                {"Connection", "Upgrade"},
                {"Sec-WebSocket-Accept", accept.c_str ()},
                {"Date", date.c_str ()},
                {"Last-Modified", date.c_str ()},
            }};

            auto const status_line =
                build_status_line (http_status_code::switching_protocols, "Switching Protocols");

            auto sender = pstore::http::net::network_sender;
            return pstore::http::send (sender, io, status_line) >>= [&] (IO io2) {
                return pstore::http::send (
                    sender, io2, build_headers (std::begin (headers), std::end (headers)));
            };
        };

        auto server_loop_thread = [&channels] (Reader && reader2, socket_descriptor io2,
                                               std::string const uri) {
            PSTORE_TRY {
                constexpr auto ident = "websocket";
                pstore::threads::set_name (ident);
                pstore::create_log_stream (ident);

                log (priority::info, "Started WebSockets session");

                PSTORE_ASSERT (io2.valid ());
                ws_server_loop (std::move (reader2), pstore::http::net::network_sender,
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
                                       &request] (socket_descriptor & s) {
            PSTORE_ASSERT (s.valid ());
            return return_type{pstore::in_place,
                               new std::thread (server_loop_thread, std::move (reader),
                                                std::move (s), request.uri ())};
        };

        PSTORE_ASSERT (io.get ().valid ());
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
            log (pstore::logger::priority::error, "error:", err);
        }
        return nread == 0 || nread == -1;
#    endif
    }
#endif

    // wait for connection
    // ~~~~~~~~~~~~~~~~~~~
    pstore::error_or<socket_descriptor> wait_for_connection (socket_descriptor const & parentfd) {
        using return_type = pstore::error_or<socket_descriptor>;

        using priority = pstore::logger::priority;

        sockaddr_in client_addr{}; // client address.
        auto clientlen = static_cast<socklen_t> (sizeof (client_addr));
        socket_descriptor childfd{::accept (parentfd.native_handle (),
                                            reinterpret_cast<struct sockaddr *> (&client_addr),
                                            &clientlen)};
        if (!childfd.valid ()) {
            return return_type{pstore::http::get_last_error ()};
        }

        // Determine who sent the message.
        PSTORE_ASSERT (clientlen == static_cast<socklen_t> (sizeof (client_addr)));
        pstore::error_or<std::string> ename = get_client_name (client_addr);
        if (!ename) {
            return return_type{pstore::http::get_last_error ()};
        }
        log (priority::info, "Connection from ", ename.get ());
        return return_type{std::move (childfd)};
    }

} // end anonymous namespace

namespace pstore {
    namespace http {

        int server (romfs::romfs & file_system, gsl::not_null<server_status *> const status,
                    channel_container const & channels,
                    std::function<void (in_port_t)> notify_listening) {
            using priority = logger::priority;

            error_or<socket_descriptor> const eparentfd = initialize_socket (status->port ());
            if (!eparentfd) {
                log (priority::error, "opening socket: ", eparentfd.get_error ().message ());
                return 0;
            }

            socket_descriptor const & parentfd = eparentfd.get ();
            status->set_real_port_number (parentfd);

            log (priority::info, "starting server-loop on port ", status->port ());

            std::vector<std::unique_ptr<std::thread>> websockets_workers;
            notify_listening (status->port ());

            for (auto expected_state = server_status::http_state::initializing;
                 status->listening (expected_state);
                 expected_state = server_status::http_state::listening) {

                // Wait for a connection request.
                error_or<socket_descriptor> echildfd = wait_for_connection (parentfd);
                if (!echildfd) {
                    log (priority::error,
                         "wait_for_connection: ", echildfd.get_error ().message ());
                    continue;
                }
                socket_descriptor & childfd = *echildfd;

                // Get the HTTP request line.
                auto reader = make_buffered_reader<socket_descriptor &> (net::refiller);

                PSTORE_ASSERT (childfd.valid ());
                error_or_n<socket_descriptor &, request_info> eri =
                    read_request (reader, std::ref (childfd));
                if (!eri) {
                    log (priority::error,
                         "Failed reading HTTP request: ", eri.get_error ().message ());
                    continue;
                }
                childfd = std::move (std::get<0> (eri));
                request_info const & request = std::get<1> (eri);
                log (priority::info, "Request: ",
                     request.method () + ' ' + request.version () + ' ' + request.uri ());

                // We only currently support the GET method.
                if (request.method () != "GET") {
                    report_error (make_error_code (pstore::http::error_code::not_implemented),
                                  request, childfd);
                    continue;
                }

                // Respond appropriately based on the request and headers.
                auto const serve_reply =
                    [&] (socket_descriptor & io2,
                         header_info const & header_contents) -> std::error_code {
                    if (header_contents.connection_upgrade &&
                        header_contents.upgrade_to_websocket) {

                        error_or<std::unique_ptr<std::thread>> p = upgrade_to_ws (
                            reader, std::ref (childfd), request, header_contents, channels);
                        if (p) {
                            websockets_workers.emplace_back (std::move (*p));
                        }
                        return p.get_error ();
                    }

                    if (!details::starts_with (request.uri (), dynamic_path)) {
                        return serve_static_content (net::network_sender, std::ref (io2),
                                                     request.uri (), file_system)
                            .get_error ();
                    }

                    return serve_dynamic_content (net::network_sender, std::ref (io2),
                                                  request.uri ())
                        .get_error ();
                };

                // Scan the HTTP headers.
                PSTORE_ASSERT (childfd.valid ());
                std::error_code const err = read_headers (
                    reader, std::ref (childfd),
                    [] (header_info io, std::string const & key, std::string const & value) {
                        return io.handler (key, value);
                    },
                    header_info ()) >>= serve_reply;

                if (err) {
                    // Report the error to the user as an HTTP error.
                    report_error (err, request, childfd);
                }

                PSTORE_ASSERT (input_is_empty (reader, childfd));
            }

            for (std::unique_ptr<std::thread> const & worker : websockets_workers) {
                worker->join ();
            }

            return 0;
        }

    } // end namespace http
} // end namespace pstore
