//*      _        _                    _ _            _    *
//*  ___| |_ __ _| |_ _   _ ___    ___| (_) ___ _ __ | |_  *
//* / __| __/ _` | __| | | / __|  / __| | |/ _ \ '_ \| __| *
//* \__ \ || (_| | |_| |_| \__ \ | (__| | |  __/ | | | |_  *
//* |___/\__\__,_|\__|\__,_|___/  \___|_|_|\___|_| |_|\__| *
//*                                                        *
//===- lib/broker_intf/status_client.cpp ----------------------------------===//
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
#include "pstore/broker_intf/status_client.hpp"

#include <cstring>
#include <string>

#ifdef _WIN32
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <sys/types.h>
#    include <sys/socket.h>
#    include <netdb.h>
#endif

#include "pstore/broker_intf/status_path.hpp"
#include "pstore/config/config.hpp"
#include "pstore/support/array_elements.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/logging.hpp"
#include "pstore/support/to_string.hpp"
#include "pstore/support/utf.hpp"

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
                getaddrinfo (node, pstore::to_string (port).c_str (), &hints, &servinfo)) {
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

    /// Create a client endpoint and connect to a server.
    socket_descriptor cli_conn (char const * node, int port) {
        auto servinfo = get_address_info (node, port);

        // servinfo now points to a linked list of 1 or more addrinfo instances.
        socket_descriptor sock_fd;
        for (auto p = servinfo.get (); p != nullptr; p = p->ai_next) {
            sock_fd.reset (::socket (p->ai_family, p->ai_socktype, p->ai_protocol));
            if (!sock_fd.valid ()) {
                // log_error (std::cerr, pstore::get_last_error (), "socket() returned");
                continue;
            }

            if (::connect (sock_fd.get (), p->ai_addr, static_cast<socklen_t> (p->ai_addrlen)) ==
                socket_descriptor::error) {
                // log_error (std::cerr, pstore::get_last_error (), "connect() returned");
                continue;
            }

            return sock_fd;
        }

        log (pstore::logging::priority::error, "unable to connect");
        return socket_descriptor{};
    }
} // end anonymous namespace

namespace pstore {
    namespace broker {

        socket_descriptor connect_to_status_server (in_port_t port) {
            log (logging::priority::info, "connecting to status server at port ", port);
            return cli_conn ("localhost", port);
        }

        socket_descriptor connect_to_status_server () {
            pstore::broker::socket_descriptor csfd;
            auto const status_file_path = pstore::broker::get_status_path ();
            in_port_t const port = pstore::broker::read_port_number_file (status_file_path);
            log (logging::priority::info, "connecting to status server at port ", port);
            // FIXME: if port is 0 file read failed.
            return connect_to_status_server (port);
        }

    } // namespace broker
} // namespace pstore
