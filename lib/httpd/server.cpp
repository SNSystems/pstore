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
#include "pstore/support/error.hpp"
#include "pstore/support/logging.hpp"


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

    struct request_info {
    public:
        request_info (std::string m, std::string v, std::string u)
                : method_{std::move (m)}
                , version_{std::move (v)}
                , uri_{std::move (u)} {}
        request_info (request_info const &) = default;
        request_info (request_info &&) = default;
        ~request_info () = default;

        request_info & operator= (request_info const &) = delete;
        request_info & operator= (request_info &&) = delete;

        std::string const & method () const { return method_; }
        std::string const & version () const { return version_; }
        std::string const & uri () const { return uri_; }

    private:
        std::string method_;
        std::string version_;
        std::string uri_;
    };

    // read_request
    // ~~~~~~~~~~~~
    template <typename ReaderType>
    pstore::error_or<request_info> read_request (ReaderType & reader, socket_descriptor & socket) {
        using pstore::error_or;
        using pstore::maybe;

        auto check_for_eof = [](std::pair<socket_descriptor &, maybe<std::string>> const & p) {
            auto const & buf = std::get<1> (p);
            if (!buf) {
                // TODO: use a more suitable error.
                return error_or<std::string>{std::make_error_code (std::errc (ENOTCONN))};
            }
            return error_or<std::string>{pstore::in_place, *buf};
        };

        auto extract_request_info = [](std::string const & s) {
            std::istringstream str{s};
            std::string method;
            std::string version;
            std::string uri;
            str >> method >> uri >> version;
            return error_or<request_info>{pstore::in_place, method, version, uri};
        };

        return (reader.gets (socket) >>= check_for_eof) >>= extract_request_info;
    }

    // read_headers
    // ~~~~~~~~~~~~
    template <typename ReaderType, typename HandleFn>
    ReaderType & read_headers (ReaderType & reader, socket_descriptor & childfd, HandleFn handler) {
        using pstore::error_or;
        using pstore::maybe;

        error_or<std::pair<socket_descriptor &, maybe<std::string>>> const es =
            reader.gets (childfd);
        if (es.has_error ()) {
            log (pstore::logging::priority::error, "recv", es.get_error ().message ());
            return reader;
        }
        assert (std::get<0> (es.get_value ()) == childfd);
        std::string const & s = std::get<1> (es.get_value ()).value ();
        if (s.length () == 0) {
            return reader;
        }
        handler (s);
        return read_headers (reader, childfd, handler);
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

} // end anonymous namespace

namespace pstore {
    namespace httpd {

        int server (in_port_t port_number, romfs::romfs & file_system) {
            logging::create_log_stream ("httpd");

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
            for (;;) {

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

#if 0
                char const * const hostaddrp = inet_ntoa (clientaddr.sin_addr);
                if (hostaddrp == nullptr) {
                    log (logging::priority::error, "inet_ntoa");
                    continue;
                }
#endif

                // Get the HTTP request line.
                auto reader = make_buffered_reader<socket_descriptor &> (refiller);

                pstore::error_or<request_info> eri = read_request (reader, childfd);
                if (eri.has_error ()) {
                    log (logging::priority::error, "reading HTTP request",
                         eri.get_error ().message ());
                    continue;
                }

                request_info const & request = eri.get_value ();
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

                /* parse the uri [crufty] */
                bool is_static = true; /* static request? */
                std::string filename;
                constexpr std::size_t buffer_size = 1024;
                char cgiargs[buffer_size];      /* cgi argument list */
                if (!strstr (request.uri ().c_str (), "cgi-bin")) { /* static content */
                    is_static = true;
                    cgiargs[0] = '\0';
                    filename = ".";
                    filename += request.uri ();
                    if (request.uri ().back () == '/') {
                        filename += "index.html";
                    }
                } else { /* dynamic content */
#if 0
                    is_static = false;
                    if (char * p = index (uri, '?')) {
                        std::strcpy (cgiargs, p + 1);
                        *p = '\0';
                    } else {
                        std::strcpy (cgiargs, "");
                    }
                    filename = ".";
                    filename += uri;
#endif
                }

                // Make sure the file exists.
                error_or<struct romfs::stat> sbuf = file_system.stat (filename.c_str ());
                if (sbuf.has_error ()) {
                    cerror (childfd, filename.c_str (), "404", "Not found",
                            "pstore-httpd couldn't find this file");
                    childfd.reset ();
                    continue;
                }

                /* serve static content */
                if (is_static) {
                    {
                        // Send the response header.
                        std::ostringstream os;
                        os << "HTTP/1.1 200 OK\nServer: pstore-httpd\n";
                        os << "Content-length: " << static_cast<unsigned> (sbuf->st_size) << '\n';
                        os << "Content-type: " << media_type_from_filename (filename) << '\n';
                        os << "\r\n";
                        send (childfd, os.str ());
                    }
                    std::array<std::uint8_t, 256> buffer{{0}};
                    error_or<romfs::descriptor> descriptor = file_system.open (filename.c_str ());
                    assert (!descriptor.has_error ());
                    for (;;) {
                        std::size_t num_read = descriptor->read (buffer.data (), sizeof (std::uint8_t), buffer.size ());
                        if (num_read == 0) {
                            break;
                        }
                        send (childfd, buffer.data (), num_read);
                    }
                } else {
                    // TODO: serve dynamic content.
                }

                /* clean up */
                childfd.reset ();
            }
        }

    } // end namespace httpd
} // end namespace pstore
