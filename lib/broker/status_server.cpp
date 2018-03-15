//*      _        _                                             *
//*  ___| |_ __ _| |_ _   _ ___   ___  ___ _ ____   _____ _ __  *
//* / __| __/ _` | __| | | / __| / __|/ _ \ '__\ \ / / _ \ '__| *
//* \__ \ || (_| | |_| |_| \__ \ \__ \  __/ |   \ V /  __/ |    *
//* |___/\__\__,_|\__|\__,_|___/ |___/\___|_|    \_/ \___|_|    *
//*                                                             *
//===- lib/broker/status_server.cpp ---------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
#include "pstore/broker/status_server.hpp"

#include <array>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <limits>
#include <map>
#include <sstream>
#include <vector>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#else
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/broker_intf/status_path.hpp"
#include "pstore/broker_intf/wsa_startup.hpp"
#include "pstore/config/config.hpp"
#include "pstore/json/dom_types.hpp"
#include "pstore/json/json.hpp"
#include "pstore/support/logging.hpp"


constexpr char EOT = 4; // ASCII "end of transmission"

#ifdef _WIN32
using ssize_t = SSIZE_T;
#endif

std::uint16_t network_to_host (std::uint16_t v) {
    return ntohs (v);
}
std::uint32_t network_to_host (std::uint32_t v) {
    return ntohl (v);
}

std::uint16_t host_to_network (std::uint16_t v) {
    return htons (v);
}
std::uint32_t host_to_network (std::uint32_t v) {
    return htonl (v);
}


namespace {

    using json_parser = pstore::json::parser<pstore::json::yaml_output>;
    using socket_descriptor = pstore::broker::socket_descriptor;

    bool handle_request (pstore::gsl::span<char const> buf, socket_descriptor::value_type clifd,
                         json_parser & parser) {
        auto report_error = [](std::error_code err) {
            assert (err);
            std::ostringstream message;
            message << err.message () << " (" << err.value () << ')';
            log (pstore::logging::priority::error,
                 "There was a JSON parsing error: ", message.str ());
            return false; // no more.
        };

        auto size = buf.size ();
        bool const is_eof = size > 0 && buf[size - 1] == EOT;
        if (is_eof) {
            --size;
        }

        parser.parse (pstore::gsl::make_span (buf.data (), size));
        if (auto const err = parser.last_error ()) {
            return report_error (err);
        }
        if (!is_eof) {
            return true; // more please!
        }

        pstore::dump::value_ptr dom = parser.eof ();
        if (auto const err = parser.last_error ()) {
            return report_error (err);
        }

        // Just send back the YAML equivalent of the result of the JSON parse.
        std::ostringstream out;
        dom->write (out);
        out << '\n';
        std::string const & str = out.str ();
        send (clifd, str.data (), str.length (), 0 /*flags*/);

        return false;
    }

} // namespace

namespace {

#ifndef _WIN32
    using platform_erc = pstore::errno_erc;
#else
    using platform_erc = pstore::win32_erc;
#endif

    pstore::broker::socket_descriptor create_socket (int domain, int type, int protocol) {
        pstore::broker::socket_descriptor fd{::socket (domain, type, protocol)};
        if (!fd.valid ()) {
            raise (platform_erc (pstore::broker::get_last_error ()), "socket creation failed");
        }
        return fd;
    }

    void bind (pstore::broker::socket_descriptor const & socket, sockaddr const * address,
               socklen_t address_len) {
        if (::bind (socket.get (), address, address_len) != 0) {
            raise (platform_erc (pstore::broker::get_last_error ()), "socket bind failed");
        }
    }

#if PSTORE_UNIX_DOMAIN_SOCKETS
    void bind (pstore::broker::socket_descriptor const & socket, sockaddr_un const & address) {
        bind (socket, reinterpret_cast<sockaddr const *> (&address), SUN_LEN (&address));
    }
#endif // PSTORE_UNIX_DOMAIN_SOCKETS

    void bind (pstore::broker::socket_descriptor const & socket, sockaddr_in6 const & address) {
        bind (socket.get (), reinterpret_cast<sockaddr const *> (&address), sizeof (address));
    }



    template <typename T>
    socket_descriptor accept (socket_descriptor const & listenfd, T * const their_addr) {
        auto addr_size = static_cast<socklen_t> (sizeof (T));
        auto client_fd = socket_descriptor{::accept (
            listenfd.get (), reinterpret_cast<struct sockaddr *> (their_addr), &addr_size)};
        if (!client_fd.valid ()) {
            raise (platform_erc (pstore::broker::get_last_error ()), "accept failed");
        }
        return client_fd;
    }

    bool is_v4_mapped_localhost (in6_addr const & addr) {
        if (IN6_IS_ADDR_V4MAPPED (&addr)) {
            static std::array<std::uint8_t, 4> const localhostv4 = {{0x7f, 0x00, 0x00, 0x01}};
            return std::equal (std::begin (localhostv4), std::end (localhostv4), &addr.s6_addr[12]);
        }
        return false;
    }

#if PSTORE_UNIX_DOMAIN_SOCKETS
    socket_descriptor server_accept_new_ud_connection (socket_descriptor const & listenfd) {
        sockaddr_storage their_addr;
        std::memset (&their_addr, 0, sizeof their_addr);
        auto client = accept (listenfd, &their_addr);
        log (pstore::logging::priority::notice, "Accepted connection ",
             client.get ()); // TODO: address?
        return client;
    }
#endif // PSTORE_UNIX_DOMAIN_SOCKETS


    // server_accept_new_connection
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Wait for a client connection to arrive, and accept it.
    socket_descriptor server_accept_new_inet_connection (socket_descriptor const & listenfd,
                                                         bool localhost_only) {
        sockaddr_storage their_addr;
        std::memset (&their_addr, 0, sizeof their_addr);

        socket_descriptor client_fd = accept (listenfd, &their_addr);

        // Decide whether or not we'll accept a connection from this client.
        constexpr auto ipstr_max_len =
            INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN;
        char ipstr[ipstr_max_len + 1] = {'\0'};
        // int port = 0;

        bool is_localhost = false;

        // deal with both IPv4 and IPv6:
        if (their_addr.ss_family == AF_INET) {
            auto v4s = reinterpret_cast<sockaddr_in *> (&their_addr);
            // port = ntohs(v4s->sin_port);
            inet_ntop (AF_INET, &v4s->sin_addr, ipstr, sizeof ipstr);
            static_assert (INADDR_LOOPBACK <= std::numeric_limits<std::uint32_t>::max (),
                           "INADDR_LOOPBACK is too large to be a uint32_t");
            is_localhost = (v4s->sin_addr.s_addr ==
                            host_to_network (static_cast<std::uint32_t> (INADDR_LOOPBACK)));
        } else if (their_addr.ss_family == AF_INET6) {
            auto v6s = reinterpret_cast<sockaddr_in6 *> (&their_addr);
            // port = ntohs(s->sin6_port);
            in6_addr const & addr6 = v6s->sin6_addr;
            inet_ntop (AF_INET6, &addr6, ipstr, sizeof ipstr);
            is_localhost =
                std::memcmp (&v6s->sin6_addr, &in6addr_loopback, sizeof (v6s->sin6_addr)) == 0 ||
                is_v4_mapped_localhost (v6s->sin6_addr);
        }

        ipstr[pstore::broker::array_elements (ipstr) - 1] = '\0'; // ensure nul termination.

        if (!localhost_only || is_localhost) {
            log (pstore::logging::priority::notice, "Accepted connection from host: ", ipstr);
        } else {
            log (pstore::logging::priority::notice,
                 "Refused connection from non-localhost host: ", ipstr);
            client_fd.reset ();
        }

        return client_fd;
    }

#if PSTORE_UNIX_DOMAIN_SOCKETS
    // server_listen_unix_domain
    // ~~~~~~~~~~~~~~~~~~~~~~~~~
    socket_descriptor server_listen_unix_domain (char const * name) {
        sockaddr_un un;
        constexpr std::size_t max_name_len = pstore::broker::array_elements (un.sun_path);
        std::memset (&un, 0, sizeof un);
        un.sun_family = AF_UNIX;
        std::strncpy (un.sun_path, name, max_name_len);
        if (un.sun_path[max_name_len - 1U] != '\0') {
            raise (pstore::errno_erc{ENAMETOOLONG}, "socket name was too long");
        }

        socket_descriptor fd = create_socket (AF_UNIX, SOCK_STREAM, 0);
        ::unlink (name); // in case it already exists

        // bind the name to the descriptor
        bind (fd, un);
        return fd;
    }
#endif // PSTORE_UNIX_DOMAIN_SOCKETS


    socket_descriptor server_listen_inet () {
        // The socket() function returns a socket descriptor, which represents an endpoint. Get a
        // socket for address family AF_INET6 to prepare to accept incoming connections on.
        socket_descriptor fd = create_socket (AF_INET6, SOCK_STREAM, 0);

        // After the socket descriptor is created, a bind() function gets a unique name for the
        // socket. We accept connections from the loopback address and let the system pick the port
        // number on which we will listen.
        sockaddr_in6 serveraddr;
        std::memset (&serveraddr, 0, sizeof serveraddr);
        serveraddr.sin6_family = AF_INET6;
        serveraddr.sin6_port = host_to_network (in_port_t{0});
        serveraddr.sin6_addr = in6addr_loopback; // in6addr_any;
        bind (fd, serveraddr);
        return fd;
    }

    in_port_t get_listen_port (socket_descriptor const & fd) {
        sockaddr_storage address;
        std::memset (&address, 0, sizeof address);
        socklen_t sock_addr_len = sizeof (address);
        if (getsockname (fd.get (), reinterpret_cast<sockaddr *> (&address), &sock_addr_len) != 0) {
            pstore::raise (platform_erc (pstore::broker::get_last_error ()), "getsockname failed");
        }

        auto listen_port = in_port_t{0};
        switch (address.ss_family) {
        case AF_INET:
            listen_port = (reinterpret_cast<sockaddr_in const *> (&address))->sin_port;
            break;
        case AF_INET6:
            listen_port = (reinterpret_cast<sockaddr_in6 *> (&address))->sin6_port;
            break;
        default:
            raise (pstore::errno_erc{EINVAL}, "unexpectedprotocol type returned by getsockname");
        }

        return network_to_host (listen_port);
    }


    bool is_recv_error (ssize_t nread) {
#ifdef _WIN32
        return nread == SOCKET_ERROR;
#else
        return nread < 0;
#endif
    }

} // end anonymous namespace

void pstore::broker::status_server (bool use_inet_socket) {
#ifdef _WIN32
    pstore::wsa_startup startup;
    if (!startup.started ()) {
        log (logging::priority::error, "WSAStartup() failed");
        //            return EXIT_FAILURE; // FIXME: need a way to bail out.
    }
#endif

    try {
        std::map<socket_descriptor, json_parser> clients;

        constexpr auto read_buffer_size = std::size_t{4}; // FIXME: stupidly small at the moment.
        std::vector<char> read_buffer (read_buffer_size, char{});

        fd_set all_set;
        FD_ZERO (&all_set);

        // obtain a file descriptor which we can use to listen for client requests.
        std::string const listen_path = get_status_path ();
        auto listen_fd =
#if PSTORE_UNIX_DOMAIN_SOCKETS
            !use_inet_socket ? server_listen_unix_domain (listen_path.c_str ()) :
#endif // PSTORE_UNIX_DOMAIN_SOCKETS
                             server_listen_inet ();

        // tell the kernel we're a server
        constexpr int qlen = 10;
        if (listen (listen_fd.get (), qlen) != 0) {
            raise (platform_erc (get_last_error ()), "listen failed");
        }

        FD_SET (listen_fd.get (), &all_set);
        auto max_fd = listen_fd.get ();

        if (use_inet_socket) {
            auto const listen_port = get_listen_port (listen_fd);
            write_port_number_file (listen_path, listen_port);
            log (logging::priority::info, "Waiting for connections on inet port ", listen_port);
        } else {
            log (logging::priority::info, "Waiting for connections on UD path ", listen_path);
        }

        // FIXME: need a way to shut this thread down cleanly.
        for (;;) {
            fd_set rset = all_set; // rset is modified each time round the loop.

            if (select (static_cast<int> (max_fd) + 1, &rset, nullptr, nullptr, nullptr) < 0) {
                log (logging::priority::error, "select error ", get_last_error ());
            }

            if (FD_ISSET (listen_fd.get (), &rset)) {
                // accept a new client request
                socket_descriptor client_fd =
#if PSTORE_UNIX_DOMAIN_SOCKETS
                    !use_inet_socket
                        ? server_accept_new_ud_connection (listen_fd)
                        :
#endif // PSTORE_UNIX_DOMAIN_SOCKETS
                        server_accept_new_inet_connection (listen_fd, true /*localhost only*/);

                if (!client_fd.valid ()) {
                    // FIXME: error handling wrong on Windows (at least0
                    log (logging::priority::error, "server_accept_new_connection error ",
                         client_fd.get ());
                    continue;
                }

                FD_SET (client_fd.get (), &all_set);
                max_fd = std::max (max_fd, client_fd.get ()); // max fd for select()

                clients.emplace (std::move (client_fd), json_parser{});
                continue;
            }

            auto it = std::begin (clients);
            while (it != std::end (clients)) {
                auto next = it;
                ++next;

                socket_descriptor const & client_fd = it->first;

                if (FD_ISSET (client_fd.get (), &rset)) {
                    // read data from the client
                    ssize_t const nread =
                        recv (client_fd.get (), read_buffer.data (),
                              static_cast<int> (read_buffer.size ()), 0 /*flags*/);
                    assert (is_recv_error (nread) ||
                            static_cast<std::size_t> (nread) <= read_buffer.size ());
                    if (is_recv_error (nread)) {
                        logging::log (logging::priority::error, "recv() error ", get_last_error ());
                        continue;
                    }

                    bool more = false;
                    if (nread == 0) {
                        // client has closed the connection.
                        logging::log (logging::priority::error,
                                      "client closed the connection before ^D was received. fd=",
                                      client_fd.get ());
                    } else {
                        // process the client's request.
                        more = handle_request (pstore::gsl::make_span (read_buffer.data (), nread),
                                               client_fd.get (), it->second);
                    }

                    // If we're done with the client, close the file descriptor.
                    if (!more) {
                        logging::log (logging::priority::notice, "Closing connection ",
                                      client_fd.get ());
                        FD_CLR (client_fd.get (), &all_set);
                        clients.erase (it);
                    }
                }

                it = next;
            }
        }

        pstore::file::unlink (listen_path);
    } catch (std::exception const & ex) {
        log (pstore::logging::priority::error, ex.what ());
    } catch (...) {
        log (pstore::logging::priority::error, "An unknown error was detected");
    }
}
// eof: lib/broker/status_server.cpp
