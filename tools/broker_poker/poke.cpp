//*              _         *
//*  _ __   ___ | | _____  *
//* | '_ \ / _ \| |/ / _ \ *
//* | |_) | (_) |   <  __/ *
//* | .__/ \___/|_|\_\___| *
//* |_|                    *
//===- tools/broker_poker/poke.cpp ----------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "pstore_support/portab.hpp"
#include "pstore_support/gsl.hpp"
#include "pstore_support/utf.hpp"
#include "pstore_broker_intf/fifo_path.hpp"
#include "pstore_broker_intf/send_message.hpp"
#include "pstore_broker_intf/writer.hpp"

#include "flood_server.hpp"
#include "switches.hpp"
#include <thread>

namespace {
    constexpr bool error_on_timeout = true;
} // (anonymous namespace)


#ifdef PSTORE_CPP_EXCEPTIONS
#define TRY try
#define CATCH(ex, handler) catch (ex) handler
#else
#define TRY
#define CATCH(ex, handler)
#endif

#ifdef _WIN32
#define NATIVE_TEXT(str) _TEXT (str)
#else
#define NATIVE_TEXT(str) str
#endif


namespace {

#ifdef PSTORE_CPP_EXCEPTIONS
    auto & error_stream =
#if defined(_WIN32) && defined(_UNICODE)
        std::wcerr;
#else
        std::cerr;
#endif
#endif // PSTORE_CPP_EXCEPTIONS

} // (anonymous namespace)


#if defined(_WIN32) && !defined(PSTORE_IS_INSIDE_LLVM)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    // TODO:Make sure that the broker is running?
    TRY {
        switches opt;
        std::tie (opt, exit_code) = get_switches (argc, argv);
        if (exit_code == EXIT_FAILURE) {
            return exit_code;
        }

        pstore::gsl::czstring pipe_path =
            opt.pipe_path.has_value () ? opt.pipe_path.value ().c_str () : nullptr;

        if (opt.flood > 0) {
            flood_server (pipe_path, opt.retry_timeout, opt.max_retries, opt.flood);
        }

        pstore::broker::fifo_path fifo (pipe_path, opt.retry_timeout, opt.max_retries);
        pstore::broker::writer wr (fifo, opt.retry_timeout, opt.max_retries);

        if (opt.verb.length () > 0) {
            char const * path_str = (opt.path.length () > 0) ? opt.path.c_str () : nullptr;
            pstore::broker::send_message (wr, error_on_timeout, opt.verb.c_str (), path_str);
        }

        if (opt.kill) {
            pstore::broker::send_message (wr, error_on_timeout, "SUICIDE", nullptr);
        }

    } CATCH (std::exception const & ex, {
        auto what = ex.what ();
        error_stream << NATIVE_TEXT ("An error occurred: ") << pstore::utf::to_native_string (what)
                     << std::endl;
        exit_code = EXIT_FAILURE;
    }) CATCH (..., {
        std::cerr << "An unknown error occurred." << std::endl;
        exit_code = EXIT_FAILURE;
    })
    return exit_code;
}
// eof: tools/broker_poker/poke.cpp
