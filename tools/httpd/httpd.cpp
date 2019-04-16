//*  _     _   _             _  *
//* | |__ | |_| |_ _ __   __| | *
//* | '_ \| __| __| '_ \ / _` | *
//* | | | | |_| |_| |_) | (_| | *
//* |_| |_|\__|\__| .__/ \__,_| *
//*               |_|           *
//===- tools/httpd/httpd.cpp ----------------------------------------------===//
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

#include <chrono>
#include <iostream>
#include <thread>

#include "pstore/broker_intf/wsa_startup.hpp"
#include "pstore/cmd_util/cl/command_line.hpp"
#include "pstore/httpd/server.hpp"
#include "pstore/support/logging.hpp"
#include "pstore/romfs/romfs.hpp"

extern pstore::romfs::romfs fs;

namespace {

    using namespace pstore::cmd_util;
    cl::opt<in_port_t> port ("port", cl::desc ("The port number on which the server will listen"),
                             cl::init (in_port_t{8080}));

    cl::alias port2 ("p", cl::desc ("Alias for --port"), cl::aliasopt (port));

} // end anonymous namespace

int main (int argc, char * argv[]) {
    int exit_code = EXIT_SUCCESS;

#ifdef _WIN32
    pstore::wsa_startup startup;
    if (!startup.started ()) {
        std::cerr << "WSAStartup() failed\n";
        return EXIT_FAILURE;
    }
#endif // _WIN32

    PSTORE_TRY {
        cl::ParseCommandLineOptions (
            argc, argv,
            "pstore httpd: A basic HTTP/WS server for testing the pstore-httpd library.\n");

        static constexpr auto ident = "main";
        pstore::threads::set_name (ident);
        pstore::logging::create_log_stream (ident);

        std::thread t ([]() {
            static constexpr auto name = "http";
            pstore::threads::set_name (name);
            pstore::logging::create_log_stream (name);

            pstore::httpd::server (port, fs);
        });
        t.join ();
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, {
        std::cerr << "Error: " << ex.what () << '\n';
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        std::cerr << "Unknown exception.\n";
        exit_code = EXIT_FAILURE;
    })
    // clang-format on
    return exit_code;
}
