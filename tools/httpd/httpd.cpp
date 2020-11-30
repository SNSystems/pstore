//*  _     _   _             _  *
//* | |__ | |_| |_ _ __   __| | *
//* | '_ \| __| __| '_ \ / _` | *
//* | | | | |_| |_| |_) | (_| | *
//* |_| |_|\__|\__| .__/ \__,_| *
//*               |_|           *
//===- tools/httpd/httpd.cpp ----------------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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

#ifndef _WIN32
#    include <signal.h>
#endif

#include "pstore/cmd_util/command_line.hpp"
#include "pstore/cmd_util/tchar.hpp"
#include "pstore/http/buffered_reader.hpp"
#include "pstore/http/net_txrx.hpp"
#include "pstore/http/server.hpp"
#include "pstore/http/server_status.hpp"
#include "pstore/http/ws_server.hpp"
#include "pstore/os/descriptor.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/os/wsa_startup.hpp"
#include "pstore/romfs/romfs.hpp"

extern pstore::romfs::romfs fs;

using namespace pstore::cmd_util;

namespace {

    cl::opt<in_port_t> port ("port", cl::desc ("The port number on which the server will listen"),
                             cl::init (in_port_t{8080}));

    cl::alias port2 ("p", cl::desc ("Alias for --port"), cl::aliasopt (port));

} // end anonymous namespace

#ifdef _WIN32
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

#ifdef _WIN32
    pstore::wsa_startup startup;
    if (!startup.started ()) {
        std::cerr << "WSAStartup() failed\n";
        return EXIT_FAILURE;
    }
#else
    signal (SIGPIPE, SIG_IGN);
#endif // _WIN32

    PSTORE_TRY {
        cl::parse_command_line_options (
            argc, argv,
            "pstore httpd: A basic HTTP/WS server for testing the pstore-httpd library.\n");

        static constexpr auto ident = "main";
        pstore::threads::set_name (ident);
        pstore::create_log_stream (ident);

        pstore::httpd::server_status status{port.get ()};
        std::thread ([&status] () {
            static constexpr auto name = "http";
            pstore::threads::set_name (name);
            pstore::create_log_stream (name);
            pstore::httpd::server (fs, &status, pstore::httpd::channel_container{});
        }).join ();
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, { // clang-format on
        error_stream << NATIVE_TEXT ("Error: ") << pstore::utf::to_native_string (ex.what ())
                     << NATIVE_TEXT ('\n');
        exit_code = EXIT_FAILURE;
    })
    // clang-format off
    PSTORE_CATCH (..., { // clang-format on
        error_stream << NATIVE_TEXT ("Unknown exception.\n");
        exit_code = EXIT_FAILURE;
    })
    // clang-format on
    return exit_code;
}
