//===- tools/broker_poker/poke.cpp ----------------------------------------===//
//*              _         *
//*  _ __   ___ | | _____  *
//* | '_ \ / _ \| |/ / _ \ *
//* | |_) | (_) |   <  __/ *
//* | .__/ \___/|_|\_\___| *
//* |_|                    *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>

// platform includes
#ifdef _WIN32
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#else
#    include <unistd.h>
#endif

#include "pstore/brokerface/fifo_path.hpp"
#include "pstore/brokerface/send_message.hpp"
#include "pstore/brokerface/writer.hpp"
#include "pstore/command_line/tchar.hpp"
#include "pstore/config/config.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp"

#include "flood_server.hpp"
#include "switches.hpp"

namespace {
    constexpr bool error_on_timeout = true;
} // namespace


#if defined(_WIN32)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    PSTORE_TRY {
        switches opt;
        std::tie (opt, exit_code) = get_switches (argc, argv);
        if (exit_code == EXIT_FAILURE) {
            return exit_code;
        }

        pstore::gsl::czstring pipe_path =
            opt.pipe_path.has_value () ? opt.pipe_path.value ().c_str () : nullptr;

        if (opt.flood > 0) {
            flood_server (pipe_path, opt.retry_timeout, opt.flood);
        }

        pstore::brokerface::fifo_path fifo (pipe_path, opt.retry_timeout,
                                            pstore::brokerface::fifo_path::infinite_retries);
        pstore::brokerface::writer wr (fifo, opt.retry_timeout,
                                       pstore::brokerface::writer::infinite_retries);

        if (opt.verb.length () > 0) {
            char const * path_str = (opt.path.length () > 0) ? opt.path.c_str () : nullptr;
            pstore::brokerface::send_message (wr, error_on_timeout, opt.verb.c_str (), path_str);
        }

        if (opt.kill) {
            pstore::brokerface::send_message (wr, error_on_timeout, "SUICIDE", nullptr);
        }
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, {
        auto what = ex.what ();
        pstore::command_line::error_stream << PSTORE_NATIVE_TEXT ("An error occurred: ")
                    << pstore::utf::to_native_string (what) << std::endl;
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        pstore::command_line::error_stream << PSTORE_NATIVE_TEXT ("An unknown error occurred.") << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format on
    return exit_code;
}
