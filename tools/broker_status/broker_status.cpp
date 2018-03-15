//*  _               _                   _        _              *
//* | |__  _ __ ___ | | _____ _ __   ___| |_ __ _| |_ _   _ ___  *
//* | '_ \| '__/ _ \| |/ / _ \ '__| / __| __/ _` | __| | | / __| *
//* | |_) | | | (_) |   <  __/ |    \__ \ || (_| | |_| |_| \__ \ *
//* |_.__/|_|  \___/|_|\_\___|_|    |___/\__\__,_|\__|\__,_|___/ *
//*                                                              *
//===- tools/broker_status/broker_status.cpp ------------------------------===//
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
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <sstream>

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/broker_intf/status_path.hpp"
#include "pstore/broker_intf/wsa_startup.hpp"
#include "pstore/config/config.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp"

#if PSTORE_IS_INSIDE_LLVM
#include "llvm/Support/CommandLine.h"
#else
#include "pstore/cmd_util/cl/command_line.hpp"
#endif

namespace {

#if PSTORE_IS_INSIDE_LLVM
    using namespace llvm;
#else
    using namespace pstore::cmd_util;
#endif

#define INET_BASE_HELP "Use internet rather than Unix domain sockets"
#if PSTORE_UNIX_DOMAIN_SOCKETS
#define INET_HELP INET_BASE_HELP
#else
#define INET_HELP INET_BASE_HELP " (ignored)"
#endif

    cl::opt<bool> use_inet_socket ("inet", cl::desc (INET_HELP), cl::init (false));

} // namespace


namespace {

#if defined(_WIN32)
    using platform_erc = pstore::win32_erc;
#else // defined (_WIN32)
    using platform_erc = pstore::errno_erc;
#endif

    class gai_error_category : public std::error_category {
    public:
        // The need for this constructor was removed by CWG defect 253 but Clang (prior to 3.9.0)
        // and GCC (before 4.6.4) require its presence.
        gai_error_category () noexcept {}
        char const * name () const noexcept override;
        std::string message (int error) const override;
    };

    char const * gai_error_category::name () const noexcept {
        return "gai error category";
    }
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

} // end anonymous namespace



namespace {

    template <typename Function>
    class on_destruct {
    public:
        explicit on_destruct (Function destroyer) : destroyer_{destroyer} {}
        on_destruct (on_destruct const &) = delete;
        on_destruct (on_destruct &&) = default;
        on_destruct & operator= (on_destruct const &) = delete;
        on_destruct & operator= (on_destruct &&) = default;

        ~on_destruct () { destroyer_ (); }
        void nop () const noexcept {}

    private:
        Function destroyer_;
    };
    template <typename Function>
    auto make_on_destruct (Function destroyer) -> on_destruct<Function> {
        return on_destruct <Function> (destroyer);
    }


    using socket_descriptor = pstore::broker::socket_descriptor;

    // Create a client endpoint and connect to a server.
#if PSTORE_UNIX_DOMAIN_SOCKETS

    socket_descriptor cli_conn (std::string const & name) {
        using namespace pstore::broker;

        // fill the socket address structure with the server′s "well-known" address.
        struct sockaddr_un un;
        std::memset (&un, 0, sizeof (un));
        un.sun_family = AF_UNIX;
        constexpr std::size_t sun_path_length = array_elements (un.sun_path);
        name.copy (un.sun_path, sun_path_length);
        if (un.sun_path[sun_path_length - 1U] != '\0') {
            raise (pstore::errno_erc {ENAMETOOLONG}, "unix domain socket name too long");
        }

        // create a UNIX domain stream socket.
        socket_descriptor fd{::socket (AF_UNIX, SOCK_STREAM, 0)};
        if (!fd.valid ()) {
            raise (platform_erc (get_last_error ()), "socket creation failed");
        }

        if (::connect (fd.get (), reinterpret_cast<struct sockaddr *> (&un), SUN_LEN (&un)) != 0) {
            raise (platform_erc {get_last_error ()}, "connect() failed");
        }
        return fd;
    }

#endif // PSTORE_UNIX_DOMAIN_SOCKETS

    socket_descriptor cli_conn (char const * node, int port) {
        //using namespace pstore;
        addrinfo * servinfo = nullptr;

        // Release servinfo when this object goes out of scope.
        auto addr_info_destroyer = make_on_destruct ([&servinfo]() noexcept {
            if (servinfo != nullptr) {
                freeaddrinfo (servinfo); // free the linked list.
                servinfo = nullptr;
            }
        });

        addrinfo hints;
        std::memset (&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;     // don't care whether we use IPv4 or IPv6.
        hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
        hints.ai_flags = AI_PASSIVE;     // fill in my IP for me.

        if (int const gai_status = getaddrinfo (node, std::to_string (port).c_str (), &hints, &servinfo)) {
#ifdef _WIN32
            pstore::raise (platform_erc{static_cast <DWORD> (WSAGetLastError())}, "getaddrinfo failed");
#else
            pstore::raise_error_code (std::error_code (gai_status, get_gai_error_category()), "getaddrinfo failed");
#endif
        }

        // servinfo now points to a linked list of 1 or more addrinfo instances.
        for (auto p = servinfo; p != nullptr; p = p->ai_next) {
            socket_descriptor::value_type sock_fd = socket (p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sock_fd == socket_descriptor::invalid) {
                //log_error (std::cerr, pstore::get_last_error (), "socket() returned");
                continue;
            }

            socket_descriptor fd {sock_fd};

            if (connect (fd.get (), p->ai_addr, static_cast<socklen_t> (p->ai_addrlen)) ==
                socket_descriptor::error) {
                //log_error (std::cerr, pstore::get_last_error (), "connect() returned");
                continue;
            }

            return fd;
        }
        return socket_descriptor{};
    }


} // end anonymous namespace

#ifdef _WIN32
int getpid () {
    return static_cast<int> (GetCurrentProcessId ());
}
#endif

#if defined(_WIN32) && defined(_UNICODE)
using pstore_tchar = wchar_t;
#else
using pstore_tchar = char;
#endif

int main (int argc, pstore_tchar * argv[]) {
    int exit_code = EXIT_SUCCESS;
    PSTORE_TRY {
#ifdef _WIN32
        pstore::wsa_startup startup;
        if (!startup.started ()) {
            std::cerr << "WSAStartup failed.\n";
            return EXIT_FAILURE;
        }
#endif

        using namespace pstore;
        cl::ParseCommandLineOptions (argc, argv, "pstore broker status utility\n");

        bool use_inet = use_inet_socket;
#if !PSTORE_UNIX_DOMAIN_SOCKETS
        use_inet = true; // Force enable inet sockets if we can't use Unix domain sockets.
#endif

        socket_descriptor csfd;
        auto const status_file_path = pstore::broker::get_status_path ();
        if (use_inet) {
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

        if (!csfd.valid ()) {
            std::cerr << "cli_conn error (" << strerror (errno) << ")\n";
            return EXIT_FAILURE;
        }

        {
            char buffer[256];
            std::snprintf (buffer, 256, "{ \"pid\": %d }\n\04", static_cast<int> (getpid ()));
            send (csfd.get (), buffer, static_cast<int> (std::strlen (buffer)), 0 /*flags*/);
        }

        // now read the server's reply.
        for (;;) {
            char buf[12];
            auto const received = recv (csfd.get (), buf, sizeof (buf), 0 /*flags*/);
            // std::cout << "received=" << received << '\n';
            if (received < 0) {
                std::cerr << "read error (" << strerror (errno) << ")\n";
                return EXIT_FAILURE;
            }
            if (received == 0) {
                // all done
                break;
            }

            assert (received > 0);
            fwrite (buf, 1U, static_cast<std::size_t> (received), stdout);
        }
    }
    PSTORE_CATCH (std::exception & ex, {
        std::cerr << "Error: " << ex.what () << '\n';
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        std::cerr << "An unknown error occurred.\n";
        exit_code = EXIT_FAILURE;
    })
    return exit_code;
}
// eof: tools/broker_status/broker_status.cpp
