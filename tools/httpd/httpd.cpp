//===- tools/httpd/httpd.cpp ----------------------------------------------===//
//*  _     _   _             _  *
//* | |__ | |_| |_ _ __   __| | *
//* | '_ \| __| __| '_ \ / _` | *
//* | | | | |_| |_| |_) | (_| | *
//* |_| |_|\__|\__| .__/ \__,_| *
//*               |_|           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <chrono>
#include <iostream>
#include <thread>

#ifndef _WIN32
#    include <signal.h>
#endif

#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/tchar.hpp"
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

using namespace pstore::command_line;

namespace {

    opt<in_port_t> http_port ("port", desc ("The port number on which the server will listen"),
                              init (in_port_t{8080}));

    alias http_port2 ("p", desc ("Alias for --port"), aliasopt (http_port));

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
        parse_command_line_options (
            argc, argv,
            "pstore httpd: A basic HTTP/WS server for testing the pstore-http library.\n");

        static constexpr auto ident = "main";
        pstore::threads::set_name (ident);
        pstore::create_log_stream (ident);

        pstore::http::server_status status{http_port.get ()};
        std::thread ([&status] () {
            static constexpr auto * const name = "http";
            pstore::threads::set_name (name);
            pstore::create_log_stream (name);
            pstore::http::server (
                fs, &status, pstore::http::channel_container{}, [] (in_port_t const port) {
                    out_stream << NATIVE_TEXT ("Listening on port ") << port << std::endl;
                });
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
