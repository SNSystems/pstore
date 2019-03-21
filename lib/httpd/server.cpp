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

#    define _WINSOCK_DEPRECATED_NO_WARNINGS
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
#include "pstore/httpd/media_type.hpp"
#include "pstore/httpd/query_to_kvp.hpp"
#include "pstore/httpd/request.hpp"
#include "pstore/support/array_elements.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/logging.hpp"
#include "pstore/support/random.hpp"

// FIXME: there are multiple definitions of this type.
#ifdef _WIN32
using ssize_t = SSIZE_T;
#endif

namespace {

    using socket_descriptor = pstore::broker::socket_descriptor;

    // FIXME: the following two functions were copied from the broker.
    // is_recv_error
    // ~~~~~~~~~~~~~
    constexpr bool is_recv_error (ssize_t nread) {
#ifdef _WIN32
        return nread == SOCKET_ERROR;
#else
        return nread < 0;
#endif // !_WIN32
    }

    // get_last_error
    // ~~~~~~~~~~~~~~
    inline std::error_code get_last_error () noexcept {
#ifdef _WIN32
        return std::make_error_code (pstore::win32_erc{static_cast<DWORD> (WSAGetLastError ())});
#else
        return std::make_error_code (std::errc (errno));
#endif // !_WIN32
    }


    void send (socket_descriptor const & socket, void const * ptr, std::size_t size) {
#ifdef _WIN32
        assert (size <= std::numeric_limits<int>::max ());
        using size_type = int;
#else
        using size_type = std::size_t;
#endif // _!WIN32
        if (::send (socket.get (), static_cast<char const *> (ptr), static_cast<size_type> (size),
                    0 /*flags*/) < 0) {
            log (pstore::logging::priority::error, "send", get_last_error ().message ());
        }
    }
    void send (socket_descriptor const & socket, std::string const & str) {
        send (socket, str.data (), str.length ());
    }
    void send (socket_descriptor const & socket, std::ostringstream const & os) {
        send (socket, os.str ());
    }
    void cerror (socket_descriptor const & socket, char const * cause, char const * error_no,
                 char const * shortmsg, char const * longmsg) {
        std::ostringstream os;
        os << "HTTP/1.1 " << error_no << ' ' << shortmsg
           << "\n"
              "Content-type: text/html\n"
              "\n"
              "<html>\n"
              "<head><title>pstore-httpd Error</title></head>\n"
              "<body>\n"
              "<h1>pstore-httpd Web Server Error</h1>\n"
              "<p>"
           << error_no << ": " << shortmsg
           << "</p>\n"
              "<p>"
           << longmsg << ": " << cause
           << "</p>\n"
              "<hr><em>The pstore-httpd Web server</em>\n"
              "</body>\n"
              "</html>\n";
        send (socket, os);
    }

    // refiller
    // ~~~~~~~~
    /// Called when the buffered_reader<> needs more characters from the data stream.
    pstore::error_or<std::pair<socket_descriptor &, pstore::gsl::span<char>::iterator>>
    refiller (socket_descriptor & socket, pstore::gsl::span<char> const & s) {
        using result_type =
            pstore::error_or<std::pair<socket_descriptor &, pstore::gsl::span<char>::iterator>>;

        auto size = s.size ();
        size = std::max (size, decltype (size){0});
#ifdef _WIN32
        assert (size < std::numeric_limits<int>::max ());
        using size_type = int;
#else
        using size_type = std::size_t;
#endif //!_WIN32
        ssize_t const nread =
            ::recv (socket.get (), s.data (), static_cast<size_type> (size), 0 /*flags*/);
        assert (is_recv_error (nread) || (nread >= 0 && nread <= size));
        if (is_recv_error (nread)) {
            return result_type{get_last_error ()};
        }
        return result_type{pstore::in_place, socket, s.begin () + nread};
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

        return eo{pstore::in_place, std::move (fd)};
    }


    pstore::error_or<std::string> get_client_name (sockaddr_in const & client_addr) {
        std::array<char, 64> host_name {{'\0'}};
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
        return pstore::error_or<std::string>{pstore::in_place, std::string{host_name.data ()}};
    }



    void serve_static_content (socket_descriptor & childfd, std::string const & path,
                               pstore::romfs::romfs & file_system) {
        pstore::error_or<struct pstore::romfs::stat> sbuf = file_system.stat (path.c_str ());
        if (sbuf.has_error ()) {
            cerror (childfd, path.c_str (), "404", "Not found",
                    "pstore-httpd couldn't find this file");
            return;
        }

        {
            // Send the response header.
            std::ostringstream os;
            os << "HTTP/1.1 200 OK\nServer: pstore-httpd\n";
            os << "Content-length: " << static_cast<unsigned> (sbuf->st_size) << '\n';
            os << "Content-type: " << pstore::httpd::media_type_from_filename (path) << '\n';
            os << "\r\n";
            send (childfd, os.str ());
        }
        std::array<std::uint8_t, 256> buffer{{0}};
        pstore::error_or<pstore::romfs::descriptor> descriptor = file_system.open (path.c_str ());
        assert (!descriptor.has_error ());
        for (;;) {
            std::size_t num_read =
                descriptor->read (buffer.data (), sizeof (std::uint8_t), buffer.size ());
            if (num_read == 0) {
                break;
            }
            send (childfd, buffer.data (), num_read);
        }
    }

    /// Producesj a 16 digit random hex number. This is sent along with a GET of /cmd/quit to
    /// dissuade the status server from trivially shutdown by a client.
    std::string make_quit_magic () {
        std::string resl;
        pstore::random_generator<unsigned> rnd;

        auto num2hex = [](unsigned a) {
            assert (a < 16);
            return static_cast<char> (a < 10 ? a + '0' : a - 10 + 'a');
        };
        resl.reserve (16);
        for (int j = 0; j < 16; ++j) {
            resl += num2hex (rnd.get (16U));
        };
        return resl;
    }

    // Returns true if the string \p s starts with the given prefix.
    bool starts_with (std::string const & s, std::string const & prefix) {
        std::string::size_type pos = 0;
        while (pos < s.length () && pos < prefix.length () && s[pos] == prefix[pos]) {
            ++pos;
        }
        return pos == prefix.length ();
    }

} // end anonymous namespace

namespace pstore {
    namespace httpd {

        std::string get_quit_magic () {
            static std::string const quit_magic = make_quit_magic ();
            return quit_magic;
        }

        struct server_state {
            bool done = false;
        };
        using error_or_server_state = pstore::error_or<server_state>;
        using query_container = std::unordered_map<std::string, std::string>;



        error_or_server_state handle_quit (server_state state, socket_descriptor const & childfd,
                                           query_container const & query) {
            auto const pos = query.find ("magic");
            if (pos != std::end (query) && pos->second == get_quit_magic ()) {
                state.done = true;
            } else {
                cerror (childfd, "Bad request", "501", "bad request", "That was a bad request");
            }
            return error_or_server_state{pstore::in_place, state};
        }



        using commands_container =
            std::unordered_map<std::string, std::function<error_or_server_state (
                                                server_state, socket_descriptor const & childfd,
                                                query_container const &)>>;

        commands_container get_commands () {
            static commands_container const commands = {{"quit", handle_quit}};
            return commands;
        }

        int server (in_port_t port_number, romfs::romfs & file_system) {
            logging::create_log_stream ("httpd");

            log (logging::priority::info, "initializing");
            pstore::error_or<socket_descriptor> eparentfd = initialize_socket (port_number);
            if (eparentfd.has_error ()) {
                log (logging::priority::error, "opening socket", eparentfd.get_error ().message ());
                return 0;
            }
            socket_descriptor const & parentfd = eparentfd.get_value ();

            // main loop: wait for a connection request, parse HTTP,
            // serve requested content, close connection.
            sockaddr_in client_addr{}; // client address.
            auto clientlen =
                static_cast<socklen_t> (sizeof (client_addr)); // byte size of client's address
            log (logging::priority::info, "starting server-loop");

            server_state state;
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
                if (ename.has_error ()) {
                    log (logging::priority::error, "getnameinfo", ename.get_error ().message ());
                    continue;
                }
                log (logging::priority::info, "Connection from ", ename.get_value ());

                // Get the HTTP request line.
                auto reader = make_buffered_reader<socket_descriptor &> (refiller);

                pstore::error_or<std::pair<socket_descriptor &, request_info>> eri =
                    read_request (reader, childfd);
                if (eri.has_error ()) {
                    log (logging::priority::error, "reading HTTP request",
                         eri.get_error ().message ());
                    continue;
                }

                request_info const & request = std::get<1> (eri.get_value ());
                log (logging::priority::info, "Request: ",
                     request.method () + ' ' + request.version () + ' ' + request.uri ());

                // We  only currently support the GET method.
                if (request.method () != "GET") {
                    cerror (childfd, request.method ().c_str (), "501", "Not Implemented",
                            "httpd does not implement this method");
                    continue;
                }

                // read (and for the moment simply print) the HTTP headers.
                read_headers (reader, childfd, [](std::string const & header) {
                    log (logging::priority::info, "header:", header);
                });


                static char const dynamic_path[] = "/cmd/";
                if (!starts_with (request.uri (), dynamic_path)) {
                    std::string filename = ".";
                    filename += request.uri ();
                    if (request.uri ().back () == '/') {
                        filename += "index.html";
                    }
                    serve_static_content (childfd, filename, file_system);
                    continue;
                }


                // Serve dynamic content.
                std::string uri = request.uri ();
                assert (starts_with (uri, dynamic_path));
                uri.erase (std::begin (uri), std::begin (uri) + array_elements (dynamic_path) - 1U);

                auto const pos = uri.find ('?');
                auto const command = uri.substr (0, pos);
                auto it = pos != std::string::npos ? std::begin (uri) + pos + 1 : std::end (uri);
                query_container arguments;
                if (it != std::end (uri)) {
                    it = query_to_kvp (it, std::end (uri), make_insert_iterator (arguments));
                }
                auto const & commands = get_commands ();
                auto command_it = commands.find (uri.substr (0, pos));
                if (command_it == std::end (commands)) {
                    // return an error
                } else {
                    error_or_server_state e_state = command_it->second (state, childfd, arguments);
                    // TODO: if an error is returned, report it to the client.
                    state = e_state.get_value ();
                }
            }

            return 0;
        }

    } // end namespace httpd
} // end namespace pstore
