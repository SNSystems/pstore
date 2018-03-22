//*      _        _                    _ _            _    *
//*  ___| |_ __ _| |_ _   _ ___    ___| (_) ___ _ __ | |_  *
//* / __| __/ _` | __| | | / __|  / __| | |/ _ \ '_ \| __| *
//* \__ \ || (_| | |_| |_| \__ \ | (__| | |  __/ | | | |_  *
//* |___/\__\__,_|\__|\__,_|___/  \___|_|_|\___|_| |_|\__| *
//*                                                        *
//===- lib/broker_intf/status_client.cpp ----------------------------------===//
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
#include "pstore/broker_intf/status_client.hpp"

#include <cstring>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#endif

#include "pstore/broker_intf/status_path.hpp"
#include "pstore/config/config.hpp"
#include "pstore/support/array_elements.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/utf.hpp"

#ifndef PSTORE_UNIX_DOMAIN_SOCKETS
#define PSTORE_UNIX_DOMAIN_SOCKETS (0)
#endif

namespace {
    class gai_error_category : public std::error_category {
    public:
        // The need for this constructor was removed by CWG defect 253 but Clang (prior to 3.9.0)
        // and GCC (before 4.6.4) require its presence.
        gai_error_category () noexcept {}
        char const * name () const noexcept override;
        std::string message (int error) const override;
    };

    char const * gai_error_category::name () const noexcept { return "gai error category"; }
    std::string gai_error_category::message (int error) const {
#if defined(_WIN32) && defined(_UNICODE)
        return pstore::utf::from_native_string (gai_strerror (error));
#else
        return gai_strerror (error);
#endif
    }

    std::error_category const & get_gai_error_category () {
        static gai_error_category const category;
        return category;
    }

    void addrinfo_deleter (addrinfo * servinfo) {
        if (servinfo != nullptr) {
            freeaddrinfo (servinfo); // free the linked list.
            servinfo = nullptr;
        }
    }


    std::unique_ptr<addrinfo, decltype (&addrinfo_deleter)> get_address_info (char const * node,
                                                                              int port) {
        addrinfo hints;
        std::memset (&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;     // don't care whether we use IPv4 or IPv6.
        hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
        hints.ai_flags = AI_PASSIVE;     // fill in my IP for me.

        addrinfo * servinfo = nullptr;
        if (int const gai_status =
                getaddrinfo (node, std::to_string (port).c_str (), &hints, &servinfo)) {
#ifdef _WIN32
            raise (pstore::win32_erc{static_cast<DWORD> (WSAGetLastError ())},
                   "getaddrinfo failed");
#else
            pstore::raise_error_code (std::error_code (gai_status, get_gai_error_category ()),
                                      "getaddrinfo failed");
#endif
        }

        return {servinfo, &addrinfo_deleter};
    }



#if defined(_WIN32)
    using platform_erc = pstore::win32_erc;
#else // defined (_WIN32)
    using platform_erc = pstore::errno_erc;
#endif

    using socket_descriptor = pstore::broker::socket_descriptor;

    // Create a client endpoint and connect to a server.
#if PSTORE_UNIX_DOMAIN_SOCKETS

    socket_descriptor cli_conn (std::string const & name) {
        using namespace pstore::broker;

        // fill the socket address structure with the server′s "well-known" address.
        struct sockaddr_un un;
        std::memset (&un, 0, sizeof (un));
        un.sun_family = AF_UNIX;
        constexpr std::size_t sun_path_length = pstore::array_elements (un.sun_path);
        name.copy (un.sun_path, sun_path_length);
        if (un.sun_path[sun_path_length - 1U] != '\0') {
            raise (pstore::errno_erc{ENAMETOOLONG}, "unix domain socket name too long");
        }

        // create a UNIX domain stream socket.
        socket_descriptor fd{::socket (AF_UNIX, SOCK_STREAM, 0)};
        if (!fd.valid ()) {
            raise (platform_erc (get_last_error ()), "socket creation failed");
        }

        if (::connect (fd.get (), reinterpret_cast<struct sockaddr *> (&un),
                       static_cast<socklen_t> (SUN_LEN (&un))) != 0) {
            raise (platform_erc{get_last_error ()}, "connect() failed");
        }
        return fd;
    }

#endif // PSTORE_UNIX_DOMAIN_SOCKETS

    socket_descriptor cli_conn (char const * node, int port) {
        auto servinfo = get_address_info (node, port);

        // servinfo now points to a linked list of 1 or more addrinfo instances.
        for (auto p = servinfo.get (); p != nullptr; p = p->ai_next) {
            socket_descriptor::value_type sock_fd =
                socket (p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sock_fd == socket_descriptor::invalid) {
                // log_error (std::cerr, pstore::get_last_error (), "socket() returned");
                continue;
            }

            socket_descriptor fd{sock_fd};

            if (connect (fd.get (), p->ai_addr, static_cast<socklen_t> (p->ai_addrlen)) ==
                socket_descriptor::error) {
                // log_error (std::cerr, pstore::get_last_error (), "connect() returned");
                continue;
            }

            return fd;
        }
        return socket_descriptor{};
    }
} // end anonymous namespace

namespace pstore {
    namespace broker {

        socket_descriptor connect_to_status_server (bool use_inet) {
            pstore::broker::socket_descriptor csfd;
            auto const status_file_path = pstore::broker::get_status_path ();
            if (use_inet || !PSTORE_UNIX_DOMAIN_SOCKETS) {
                in_port_t const port = pstore::broker::read_port_number_file (status_file_path);
                // TODO: if port is 0 file read failed.
                csfd = cli_conn ("localhost", port);
            } else {
#if PSTORE_UNIX_DOMAIN_SOCKETS
                csfd = cli_conn (status_file_path);
#else
                assert (false);
#endif // PSTORE_UNIX_DOMAIN_SOCKETS
            }

            return csfd;
        }

    } // namespace broker
} // namespace pstore
// eof: lib/broker_intf/status_client.cpp
