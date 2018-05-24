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
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/broker_intf/status_client.hpp"
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

#ifdef _WIN32
    int getpid () { return static_cast<int> (GetCurrentProcessId ()); }
    using send_size_type = int;
#else
    using send_size_type = std::size_t;
#endif

} // namespace

#if defined(_WIN32) && !defined(PSTORE_IS_INSIDE_LLVM)
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

        pstore::broker::socket_descriptor csfd = pstore::broker::connect_to_status_server ();
        if (!csfd.valid ()) {
            std::cerr << "cli_conn error (" << strerror (errno) << ")\n";
            return EXIT_FAILURE;
        }

        {
            char buffer[256];
            std::snprintf (buffer, 256, "{ \"pid\": %d }\n\04", static_cast<int> (getpid ()));
            if (::send (csfd.get (), buffer, static_cast<send_size_type> (std::strlen (buffer)),
                        0 /*flags*/) != 0) {
#ifdef _WIN32
                raise (pstore::win32_erc{static_cast<DWORD> (WSAGetLastError ())}, "send failed");
#else
                raise (pstore::errno_erc{errno}, "send failed");
#endif
            }
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
    PSTORE_CATCH (std::exception const & ex, {
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
