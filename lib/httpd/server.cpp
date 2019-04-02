//*                                *
//*  ___  ___ _ ____   _____ _ __  *
//* / __|/ _ \ '__\ \ / / _ \ '__| *
//* \__ \  __/ |   \ V /  __/ |    *
//* |___/\___|_|    \_/ \___|_|    *
//*                                *
//===- lib/httpd/server.cpp -----------------------------------------------===//
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
#include "pstore/httpd/server.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>

#ifdef _WIN32

#    include <io.h>
#    include <winsock2.h>
#    include <ws2tcpip.h>

#else

#    include <arpa/inet.h>
#    include <fcntl.h>
#    include <netdb.h>
#    include <netinet/in.h>
#    include <sys/mman.h>
#    include <sys/socket.h>
#    include <sys/socket.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <unistd.h>

#endif

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/httpd/buffered_reader.hpp"
#include "pstore/httpd/headers.hpp"
#include "pstore/httpd/media_type.hpp"
#include "pstore/httpd/net_txrx.hpp"
#include "pstore/httpd/query_to_kvp.hpp"
#include "pstore/httpd/quit.hpp"
#include "pstore/httpd/request.hpp"
#include "pstore/httpd/send.hpp"
#include "pstore/httpd/serve_static_content.hpp"
#include "pstore/httpd/wskey.hpp"
#include "pstore/support/array_elements.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/logging.hpp"
#include "pstore/support/random.hpp"

namespace {

    using socket_descriptor = pstore::broker::socket_descriptor;

    // get_last_error
    // ~~~~~~~~~~~~~~
    inline std::error_code get_last_error () noexcept {
#ifdef _WIN32
        return make_error_code (pstore::win32_erc{static_cast<DWORD> (WSAGetLastError ())});
#else
        return make_error_code (std::errc (errno));
#endif // !_WIN32
    }


    template <typename Sender, typename IO>
    pstore::error_or<IO> cerror (Sender sender, IO io, char const * cause, unsigned error_no,
                                 char const * shortmsg, char const * longmsg) {
        static constexpr auto crlf = pstore::httpd::crlf;
        std::ostringstream os;
        os << "HTTP/1.1 " << error_no << ' ' << shortmsg << crlf << "Content-type: text/html"
           << crlf << crlf;

        os << "<!DOCTYPE html>\n"
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
        return pstore::httpd::send (sender, io, os);
    }

    // initialize_socket
    // ~~~~~~~~~~~~~~~~~
    pstore::error_or<socket_descriptor> initialize_socket (in_port_t port_number) {
        using eo = pstore::error_or<socket_descriptor>;

        socket_descriptor fd{::socket (AF_INET, SOCK_STREAM, 0)};
        if (!fd.valid ()) {
            return eo{get_last_error ()};
        }

        int const optval = 1;
        if (::setsockopt (fd.get (), SOL_SOCKET, SO_REUSEADDR,
                          reinterpret_cast<char const *> (&optval), sizeof (optval))) {
            return eo{get_last_error ()};
        }

        sockaddr_in server_addr{}; // server's addr.
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl (INADDR_ANY); // NOLINT
        server_addr.sin_port = htons (port_number);       // NOLINT
        if (::bind (fd.get (), reinterpret_cast<sockaddr *> (&server_addr), sizeof (server_addr)) <
            0) {
            return eo{get_last_error ()};
        }

        // Get ready to accept connection requests.
        if (::listen (fd.get (), 5) < 0) { // allow 5 requests to queue up.
            return eo{get_last_error ()};
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
            return pstore::error_or<std::string>{get_last_error ()};
        }
        host_name.back () = '\0'; // guarantee nul termination.
        return pstore::error_or<std::string>{std::string{host_name.data ()}};
    }




    // Returns true if the string \p s starts with the given prefix.
    bool starts_with (std::string const & s, std::string const & prefix) {
        std::string::size_type pos = 0;
        while (pos < s.length () && pos < prefix.length () && s[pos] == prefix[pos]) {
            ++pos;
        }
        return pos == prefix.length ();
    }



    struct server_state {
        bool done = false;
    };
    using error_or_server_state = pstore::error_or<server_state>;
    using query_container = std::unordered_map<std::string, std::string>;



    error_or_server_state handle_quit (server_state state, socket_descriptor & childfd,
                                       query_container const & query) {
        static constexpr auto crlf = pstore::httpd::crlf;

        auto const pos = query.find ("magic");
        if (pos != std::end (query) && pos->second == pstore::httpd::get_quit_magic ()) {
            state.done = true;

            char const quit_message[] = "<!DOCTYPE html>\n"
                                        "<html>\n"
                                        "<head><title>pstore-httpd Exiting</title></head>\n"
                                        "<body><h1>pstore-httpd Exiting</h1></body>\n"
                                        "</html>\n";
            std::ostringstream os;
            os << "HTTP/1.1 200 OK\nServer: pstore-httpd" << crlf
               << "Content-length: " << pstore::array_elements (quit_message) - 1U << crlf
               << "Content-type: text/plain" << crlf << crlf << quit_message;
            pstore::httpd::send (pstore::httpd::net::network_sender,
                                 std::reference_wrapper<socket_descriptor> (childfd),
                                 os.str ()); // TODO: errors!
        } else {
            cerror (pstore::httpd::net::network_sender, std::ref (childfd), "Bad request", 501,
                    "bad request",
                    "That was a bad request"); // TODO: errors!
        }
        return error_or_server_state{state};
    }



    using commands_container = std::unordered_map<
        std::string, std::function<error_or_server_state (server_state, socket_descriptor & childfd,
                                                          query_container const &)>>;

    commands_container const & get_commands () {
        static commands_container const commands = {{"quit", handle_quit}};
        return commands;
    }


    template <typename T>
    auto clamp_to_signed_max (T v) noexcept -> typename std::make_signed<T>::type {
        using st = typename std::make_signed<T>::type;
        constexpr auto maxs = std::numeric_limits<st>::max ();
        PSTORE_STATIC_ASSERT (maxs >= 0);
        return static_cast<st> (std::min (static_cast<T> (maxs), v));
    }


    static char const dynamic_path[] = "/cmd/";

    pstore::error_or<server_state>
    serve_dynamic_content (std::string uri, socket_descriptor & childfd, server_state state) {
        assert (starts_with (uri, dynamic_path));
        uri.erase (std::begin (uri), std::begin (uri) + pstore::array_elements (dynamic_path) - 1U);

        auto const pos = uri.find ('?');
        auto const command = uri.substr (0, pos);
        auto it = pos != std::string::npos ? std::begin (uri) + clamp_to_signed_max (pos + 1)
                                           : std::end (uri);
        query_container arguments;
        if (it != std::end (uri)) {
            it = query_to_kvp (it, std::end (uri), pstore::httpd::make_insert_iterator (arguments));
        }
        auto const & commands = get_commands ();
        auto command_it = commands.find (uri.substr (0, pos));
        if (command_it == std::end (commands)) {
            // TODO: return an error
        } else {
            error_or_server_state e_state = command_it->second (state, childfd, arguments);
            // TODO: if an error is returned, report it to the client.
            state = e_state.get ();
        }
        return pstore::error_or<server_state>{state};
    }

    // Here we bridge from the std::error_code world to HTTP status codes.
    void report_error (std::error_code error, pstore::httpd::request_info const & request,
                       socket_descriptor & socket) {
        auto report = [&error, &request, &socket](unsigned code, char const * message) {
            cerror (pstore::httpd::net::network_sender, std::ref (socket), request.uri ().c_str (),
                    code, message, error.message ().c_str ());
        };

        log (pstore::logging::priority::error, "Error:", error.message ());
        if (error == pstore::romfs::error_code::enoent ||
            error == pstore::romfs::error_code::enotdir) {
            report (404, "Not found");
        } else {
            report (501, "Server internal error");
        }
    }


    pstore::error_or<socket_descriptor &>
    upgrade_to_ws (pstore::httpd::header_info const & header_contents, socket_descriptor & socket) {
        assert (header_contents.connection_upgrade && header_contents.upgrade_to_websocket);

        // Validate the request headers/
        if (!header_contents.websocket_key || !header_contents.websocket_version) {
            return cerror (pstore::httpd::net::network_sender, std::ref (socket),
                           "Missing HTTP header", 400, "Bad request", "Missing HTTP header");
        }

        if (*header_contents.websocket_version != 13) {
            // send back a "Sec-WebSocket-Version: 13" header along with a 400 error.
        }


        // send back the server handshake response.

        // HTTP/1.1 101 Switching Protocols
        // Upgrade: websocket
        // Connection: Upgrade
        // Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
        static constexpr auto crlf = pstore::httpd::crlf;
        std::ostringstream os;
        os << "HTTP/1.1 101 Switching Protocols" << crlf << "Upgrade: websocket" << crlf
           << "Connection: upgrade" << crlf
           << "Sec-WebSocket-Accept: " << pstore::httpd::source_key (*header_contents.websocket_key)
           << crlf << crlf;
        log (pstore::logging::priority::info, "Sending:", os.str ());
        return pstore::httpd::send (pstore::httpd::net::network_sender, std::ref (socket), os) >>=
               [&](socket_descriptor & s) { return pstore::error_or<socket_descriptor &> (s); };
    }

} // end anonymous namespace

namespace pstore {
    namespace httpd {

        int server (in_port_t port_number, romfs::romfs & file_system) {
            log (logging::priority::info, "initializing");
            pstore::error_or<socket_descriptor> eparentfd = initialize_socket (port_number);
            if (!eparentfd) {
                log (logging::priority::error, "opening socket", eparentfd.get_error ().message ());
                return 0;
            }
            socket_descriptor const & parentfd = eparentfd.get ();

            // main loop: wait for a connection request, parse HTTP,
            // serve requested content, close connection.
            sockaddr_in client_addr{}; // client address.
            auto clientlen =
                static_cast<socklen_t> (sizeof (client_addr)); // byte size of client's address
            log (logging::priority::info, "starting server-loop");

            server_state state{};
            while (!state.done) {

                // Wait for a connection request.
                // TODO: some kind of shutdown procedure.
                socket_descriptor childfd{
                    ::accept (parentfd.get (), reinterpret_cast<struct sockaddr *> (&client_addr),
                              &clientlen)};
                if (!childfd.valid ()) {
                    log (logging::priority::error, "accept", get_last_error ().message ());
                    continue;
                }

                // Determine who sent the message.
                pstore::error_or<std::string> ename = get_client_name (client_addr);
                if (!ename) {
                    log (logging::priority::error, "getnameinfo", ename.get_error ().message ());
                    continue;
                }
                log (logging::priority::info, "Connection from ", ename.get ());

                // Get the HTTP request line.
                auto reader = make_buffered_reader<socket_descriptor &> (net::refiller);

                pstore::error_or<std::pair<socket_descriptor &, request_info>> eri =
                    read_request (reader, childfd);
                if (!eri) {
                    log (logging::priority::error, "reading HTTP request",
                         eri.get_error ().message ());
                    continue;
                }

                request_info const & request = std::get<1> (eri.get ());
                log (logging::priority::info, "Request: ",
                     request.method () + ' ' + request.version () + ' ' + request.uri ());

                // We only currently support the GET method.
                if (request.method () != "GET") {
                    cerror (pstore::httpd::net::network_sender, std::ref (childfd),
                            request.method ().c_str (), 501, "Not Implemented",
                            "httpd does not implement this method");
                    continue;
                }

                // Scan the HTTP headers.
                error_or<server_state> eo = read_headers (
                    reader, childfd,
                    [](header_info io, std::string const & key, std::string const & value) {
                        log (logging::priority::info, "header:", key + ": " + value);
                        return io.handler (key, value);
                    },
                    header_info ()) >>=
                    [&](std::pair<socket_descriptor &, header_info> const & res) {
                        header_info const & header_contents = std::get<1> (res);

                        if (header_contents.connection_upgrade &&
                            header_contents.upgrade_to_websocket) {

                            // TODO: here we go into the WebSockets world!
                            log (logging::priority::info, "WebSocket upgrade requested");

                            return upgrade_to_ws (header_contents, childfd) >>=
                                   [&state](socket_descriptor &) {
                                       return error_or<server_state> (state);
                                   };
                        }

                        if (!starts_with (request.uri (), dynamic_path)) {
                            return serve_static_content (pstore::httpd::net::network_sender,
                                                         std::ref (std::get<0> (res)),
                                                         request.uri (), file_system) >>=
                                   [&state](socket_descriptor &) {
                                       return error_or<server_state> (state);
                                   };
                        }

                        return serve_dynamic_content (request.uri (), std::get<0> (res), state);
                    };

                if (eo) {
                    state = *eo;
                } else {
                    // Report the error to the user as an HTTP error.
                    report_error (eo.get_error (), request, childfd);
                }
            }

            return 0;
        }

    } // end namespace httpd
} // end namespace pstore
