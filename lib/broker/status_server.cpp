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
#include <iostream>
#include <map>
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
#include "pstore/support/logging.hpp"

constexpr char EOT = 4; // ASCII "end of transmission"

#ifdef _WIN32
using ssize_t = SSIZE_T;
#endif

namespace {

    using socket_descriptor = pstore::broker::socket_descriptor;

    bool handle_request (char const * buf, std::size_t nread, socket_descriptor::value_type clifd,
                         int & state) {
        printf ("state=%d client message: ", state);
        fwrite (buf, 1, nread, stdout);
        printf ("\n");
        ++state;
        if (buf[nread - 1] != EOT) {
            return true;
        }

        auto now = std::chrono::system_clock::now ();
        std::time_t now_time = std::chrono::system_clock::to_time_t (now);
        char const * str = std::ctime (&now_time);

        send (clifd, str, static_cast<int> (std::strlen (str)), 0 /*flags*/);
        char cr = '\n';
        send (clifd, &cr, sizeof (cr), 0 /*flags*/);
        return false;
    }

} // namespace

namespace {

#ifndef _WIN32
    using platform_erc = pstore::errno_erc;
    using reuseaddr_opt_type = int;
#else
    using platform_erc = pstore::win32_erc;
    using reuseaddr_opt_type = BOOL;
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

#else

    void bind (pstore::broker::socket_descriptor const & socket, sockaddr_in6 const & address) {
        bind (socket.get (), reinterpret_cast<sockaddr const *> (&address), sizeof (address));
    }

#endif // PSTORE_UNIX_DOMAIN_SOCKETS


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

#if !PSTORE_UNIX_DOMAIN_SOCKETS
    bool is_v4_mapped_localhost (in6_addr const & addr) {
        if (IN6_IS_ADDR_V4MAPPED (&addr)) {
            static std::array<std::uint8_t, 4> const localhostv4 = {{0x7f, 0x00, 0x00, 0x01}};
            return std::equal (std::begin (localhostv4), std::end (localhostv4), &addr.s6_addr[12]);
        }
        return false;
    }
#endif

    // Wait for a client connection to arrive, and accept it.
    // Returns a new fd if all OK, <0 on error
    socket_descriptor server_accept_new_connection (socket_descriptor const & listenfd, bool localhost_only) {
        sockaddr_storage their_addr;
        std::memset (&their_addr, 0, sizeof their_addr);

        socket_descriptor client_fd = accept (listenfd, &their_addr);

#if PSTORE_UNIX_DOMAIN_SOCKETS
        (void) localhost_only;
#else
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
            is_localhost = (v4s->sin_addr.s_addr == htonl (INADDR_LOOPBACK));
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
#endif // PSTORE_UNIX_DOMAIN_SOCKETS

        return client_fd;
    }

#if PSTORE_UNIX_DOMAIN_SOCKETS

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

#else // PSTORE_UNIX_DOMAIN_SOCKETS

    socket_descriptor server_listen_inet (int port) {
        // The socket() function returns a socket descriptor, which represents an endpoint. Get a
        // socket for address family AF_INET6 to prepare to accept incoming connections on.
        socket_descriptor fd = create_socket (AF_INET6, SOCK_STREAM, 0);

        // The setsockopt() function is used to allow the local address to be reused when the server
        // is restarted before the required wait time expires.
        reuseaddr_opt_type reuse = 1;
        if (::setsockopt (fd.get (), SOL_SOCKET, SO_REUSEADDR,
                          reinterpret_cast<char const *> (&reuse), sizeof reuse) != 0) {
            pstore::raise (platform_erc (pstore::broker::get_last_error ()),
                           "setsockopt(SO_REUSEADDR) failed");
        }

        // After the socket descriptor is created, a bind() function gets a unique name for the
        // socket. In this example, the user sets the address to in6addr_any, which (by default)
        // allows connections to be established from any IPv4 or IPv6 client that specifies our port
        // number (that is, the bind is done to both the IPv4 and IPv6 TCP/IP stacks). This behavior
        // can be modified using the IPPROTO_IPV6 level socket option IPV6_V6ONLY if required.
        struct sockaddr_in6 serveraddr;
        std::memset (&serveraddr, 0, sizeof serveraddr);
        serveraddr.sin6_family = AF_INET6;
        serveraddr.sin6_port = htons (port);
        serveraddr.sin6_addr = in6addr_any;
        bind (fd, serveraddr);
        return fd;
    }

#endif // PSTORE_UNIX_DOMAIN_SOCKETS

    bool is_recv_error (ssize_t nread) {
#ifdef _WIN32
        return nread == SOCKET_ERROR;
#else
        return nread < 0;
#endif
    }



} // end anonymous namespace

void pstore::broker::status_server () {
#ifdef _WIN32
    pstore::wsa_startup startup;
    if (!startup.started ()) {
        log (logging::priority::error, "WSAStartup() failed");
        //            return EXIT_FAILURE; // FIXME: need a way to bail out.
    }
#endif

    try {
        std::map<socket_descriptor, int> clients;

        constexpr auto read_buffer_size = std::size_t{4}; // FIXME: stupidly small at the moment.
        std::vector<char> read_buffer (read_buffer_size, char{});

        fd_set all_set;
        FD_ZERO (&all_set);

        // obtain a file descriptior which we can use to listen for client requests.
#if PSTORE_UNIX_DOMAIN_SOCKETS
        std::string const listen_address = get_status_path ();
        auto listen_fd = server_listen_unix_domain (listen_address.c_str ());
#else  // PSTORE_UNIX_DOMAIN_SOCKETS
        int listen_address = MYPORT; // FIXME: don't hardwire the port number.
        auto listen_fd = server_listen_inet (listen_address);
#endif // PSTORE_UNIX_DOMAIN_SOCKETS

        // tell the kernel we're a server
        constexpr int qlen = 10;
        if (listen (listen_fd.get (), qlen) != 0) {
            raise (platform_erc (get_last_error ()), "listen failed");
        }

        FD_SET (listen_fd.get (), &all_set);
        auto max_fd = listen_fd.get ();

        log (logging::priority::info, "Waiting for connections on ", listen_address);
        // FIXME: need a way to shut this thread down cleanly.
        for (;;) {
            fd_set rset = all_set; // rset is modified each time round the loop.

            if (select (static_cast<int> (max_fd) + 1, &rset, nullptr, nullptr, nullptr) < 0) {
                log (logging::priority::error, "select error ", get_last_error ());
            }

            if (FD_ISSET (listen_fd.get (), &rset)) {
                // accept a new client request
                socket_descriptor client_fd = server_accept_new_connection (listen_fd, true /*localhost only*/);
                if (!client_fd.valid ()) {
                    // FIXME: error handling wrong on Windows (at least0
                    log (logging::priority::error, "server_accept_new_connection error ",
                         client_fd.get ());
                    continue;
                }

                FD_SET (client_fd.get (), &all_set);
                max_fd = std::max (max_fd, client_fd.get ()); // max fd for select()

                clients.emplace (std::move (client_fd), 0);
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
                        more =
                            handle_request (read_buffer.data (), static_cast<std::size_t> (nread),
                                            client_fd.get (), it->second);
                    }

                    // If we're done with the client, close the file descriptor.
                    if (!more) {
                        FD_CLR (client_fd.get (), &all_set);
                        clients.erase (it);
                    }
                }

                it = next;
            }
        }
    } catch (std::exception const & ex) {
        log (pstore::logging::priority::error, ex.what ());
    } catch (...) {
        log (pstore::logging::priority::error, "An unknown error was detected");
    }
}
// eof: lib/broker/status_server.cpp
