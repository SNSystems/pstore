//*              _         *
//*  _ __   ___ | | _____  *
//* | '_ \ / _ \| |/ / _ \ *
//* | |_) | (_) |   <  __/ *
//* | .__/ \___/|_|\_\___| *
//* |_|                    *
//===- tools/broker_poker/poke.cpp ----------------------------------------===//
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
#include "pstore/cmd_util/tchar.hpp"
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
        pstore::cmd_util::error_stream << NATIVE_TEXT ("An error occurred: ")
                    << pstore::utf::to_native_string (what) << std::endl;
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        pstore::cmd_util::error_stream << NATIVE_TEXT ("An unknown error occurred.") << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format on
    return exit_code;
}
