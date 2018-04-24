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

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <limits>
#include <sstream>
#include <tuple>
#include <unordered_map>
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

#include "pstore/broker/globals.hpp"
#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/broker_intf/status_client.hpp"
#include "pstore/broker_intf/status_path.hpp"
#include "pstore/broker_intf/wsa_startup.hpp"
#include "pstore/config/config.hpp"
#include "pstore/json/dom_types.hpp"
#include "pstore/json/json.hpp"
#include "pstore/support/array_elements.hpp"
#include "pstore/support/logging.hpp"

namespace {

    constexpr char EOT = 4; // ASCII "end of transmission"

#ifdef _WIN32
using ssize_t = SSIZE_T;
#endif

std::uint16_t network_to_host (std::uint16_t v) {
    return ntohs (v);
}
// std::uint32_t network_to_host (std::uint32_t v) { return ntohl (v); }

std::uint16_t host_to_network (std::uint16_t v) {
    return htons (v);
}
std::uint32_t host_to_network (std::uint32_t v) {
    return htonl (v);
}

} // end of anonymous namespace

namespace {

    using json_parser = pstore::json::parser<pstore::json::null_output>;
    using socket_descriptor = pstore::broker::socket_descriptor;

    std::tuple<bool, bool> handle_request (pstore::gsl::span<char const> buf,
                                           socket_descriptor::value_type clifd,
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
            return std::make_tuple (report_error (err), false);
        }
        if (!is_eof) {
            return std::make_tuple (true, false); // more please!
        }

#if 0
        pstore::json::dom::value_ptr dom = parser.eof ();
        if (auto const err = parser.last_error ()) {
            return std::make_tuple (report_error (err), false);
        }

        if (auto obj = dom->dynamic_cast_object ()) {
            if (auto quit = obj->get ("quit")) {
                if (auto v = quit->dynamic_cast_boolean ()) {
                    bool const exit = v->get ();
                    log (pstore::logging::priority::info,
                         "Received valid 'quit' message. Exit=", exit ? "true" : "false");
                    return std::make_tuple (false /*more?*/, exit);
                }
            }
        }

        // Just send back the YAML equivalent of the result of the JSON parse.
        std::ostringstream out;
        //dom->write (out); // FIXME: produce something!!!
        out << '\n';
        std::string const & str = out.str ();
        send (clifd, str.data (), str.length (), 0 /*flags*/);

        return std::make_tuple (false /*more?*/, false /*exit?*/);
#else
        // FIXME: just exit if there's a request of any kind!
        return std::make_tuple (false /*more?*/, true);
#endif

    }

} // end anonymous namespace

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

    // server_accept_new_connection
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Wait for a client connection to arrive, and accept it.
    socket_descriptor server_accept_connection (socket_descriptor const & listenfd,
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
        switch (their_addr.ss_family) {
        case AF_INET: {
            auto v4s = reinterpret_cast<sockaddr_in const *> (&their_addr);
            // port = ntohs(v4s->sin_port);
            inet_ntop (AF_INET, &v4s->sin_addr, ipstr, sizeof ipstr);
            static_assert (INADDR_LOOPBACK <= std::numeric_limits<std::uint32_t>::max (),
                           "INADDR_LOOPBACK is too large to be a uint32_t");
            is_localhost = (v4s->sin_addr.s_addr ==
                            host_to_network (static_cast<std::uint32_t> (INADDR_LOOPBACK)));
        } break;

        case AF_INET6: {
            auto v6s = reinterpret_cast<sockaddr_in6 const *> (&their_addr);
            // port = ntohs(v6s->sin6_port);
            in6_addr const & addr6 = v6s->sin6_addr;
            inet_ntop (AF_INET6, &addr6, ipstr, sizeof ipstr);
            is_localhost =
                std::memcmp (&v6s->sin6_addr, &in6addr_loopback, sizeof (v6s->sin6_addr)) == 0 ||
                is_v4_mapped_localhost (v6s->sin6_addr);
        } break;
        }

        ipstr[pstore::array_elements (ipstr) - 1] = '\0'; // ensure nul termination.

        if (!localhost_only || is_localhost) {
            log (pstore::logging::priority::notice, "Accepted connection from host: ", ipstr);
        } else {
            log (pstore::logging::priority::notice,
                 "Refused connection from non-localhost host: ", ipstr);
            client_fd.reset ();
        }

        return client_fd;
    }

    // create_server_listen_socket
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~
    socket_descriptor create_server_listen_socket () {
        // Get a socket for address family AF_INET6 to prepare to accept incoming connections.
        socket_descriptor sock = create_socket (AF_INET6, SOCK_STREAM, 0);

        // After the socket descriptor is created, bind() gets a unique name for the socket. We
        // accept connections from the loopback address and let the system pick the port number on
        // which we will listen.
        sockaddr_in6 address;
        std::memset (&address, 0, sizeof address);
        address.sin6_family = AF_INET6;
        address.sin6_port = host_to_network (in_port_t{0});
        address.sin6_addr = in6addr_any;
        if (::bind (sock.get (), reinterpret_cast<sockaddr const *> (&address), sizeof (address)) !=
            0) {
            raise (platform_erc (pstore::broker::get_last_error ()), "socket bind failed");
        }
        return sock;
    }

    // get_listen_port
    // ~~~~~~~~~~~~~~~
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

    // is_recv_error
    // ~~~~~~~~~~~~~
    constexpr bool is_recv_error (ssize_t nread) {
#ifdef _WIN32
        return nread == SOCKET_ERROR;
#else
        return nread < 0;
#endif // !_WIN32
    }

} // end anonymous namespace


in_port_t pstore::broker::self_client_connection::get_port () const {
    std::unique_lock<decltype (mut_)> lock{mut_};
    if (!port_) {
        cv_.wait (lock, [this]() -> bool { return port_.has_value (); });
    }
    return *port_;
}

void pstore::broker::self_client_connection::set_port (in_port_t port) {
    {
        std::lock_guard<decltype (mut_)> lock{mut_};
        assert (!port_);
        port_ = port;
    }
    cv_.notify_one ();
}


void pstore::broker::status_server (std::shared_ptr<self_client_connection> client_ptr) {
#ifdef _WIN32
    pstore::wsa_startup startup;
    if (!startup.started ()) {
        log (logging::priority::error, "WSAStartup() failed");
        //            return EXIT_FAILURE; // FIXME: need a way to bail out.
    }
#endif // _WIN32

    try {

        // Now create a socket that the server will use to listen for incoming connections.
        socket_descriptor listen_fd = create_server_listen_socket ();

        // Tell the kernel we're a server
        constexpr int qlen = 10;
        if (listen (listen_fd.get (), qlen) != 0) {
            raise (platform_erc (get_last_error ()), "listen failed");
        }

        in_port_t const listen_port = get_listen_port (listen_fd);
        std::string const listen_path = get_status_path ();
        write_port_number_file (listen_path, listen_port);
        file::deleter pfdeleter (listen_path); // delete the file on destruction

        std::unordered_map<socket_descriptor, json_parser> clients;

        fd_set all_set;
        FD_ZERO (&all_set);
        FD_SET (listen_fd.get (), &all_set);
        auto max_fd = listen_fd.get ();

        log (logging::priority::info, "Waiting for connections on inet port ", listen_port);
        // Notify anyone interested that the server now has a port number.
        client_ptr->set_port (listen_port);

        while (!done) {
            fd_set rset = all_set; // rset is modified each time round the loop.

            if (::select (static_cast<int> (max_fd) + 1, &rset, nullptr, nullptr, nullptr) < 0) {
                log (logging::priority::error, "select error ", get_last_error ());
            }

            if (FD_ISSET (listen_fd.get (), &rset)) {
                socket_descriptor client_fd =
                    server_accept_connection (listen_fd, true /*localhost only?*/);
                if (!client_fd.valid ()) {
                    log (logging::priority::error, "server_accept_connection error ",
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
                    std::array<char, 4> read_buffer; // FIXME: stupidly small at the moment.

                    // read data from the client
                    ssize_t const nread =
                        recv (client_fd.get (), read_buffer.data (),
                              static_cast<int> (read_buffer.size ()), 0 /*flags*/);
                    assert (is_recv_error (nread) ||
                            static_cast<std::size_t> (nread) <= read_buffer.size ());
                    if (is_recv_error (nread)) {
                        log (logging::priority::error, "recv() error ", get_last_error ());
                        continue;
                    }

                    bool more = false;
                    if (nread == 0) {
                        // client has closed the connection.
                        log (logging::priority::error,
                             "client closed the connection before ^D was received. fd=",
                             client_fd.get ());
                    } else {
                        // process the client's request.
                        std::tie (more, done) =
                            handle_request (pstore::gsl::make_span (read_buffer.data (), nread),
                                            client_fd.get (), it->second);
                        assert (!done || (done && !more));
                    }

                    // If we're done with the client, close the file descriptor.
                    if (!more) {
                        log (logging::priority::notice, "Closing connection ", client_fd.get ());
                        FD_CLR (client_fd.get (), &all_set);
                        clients.erase (it);
                    }
                }

                it = next;
            }
        }

        log (pstore::logging::priority::info, "Status server exiting");
    } catch (std::exception const & ex) {
        log (pstore::logging::priority::error, ex.what ());
    } catch (...) {
        log (pstore::logging::priority::error, "An unknown error was detected");
    }
}
// eof: lib/broker/status_server.cpp
