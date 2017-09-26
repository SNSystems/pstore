//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===- tools/vacuum/main.cpp ----------------------------------------------===//
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
/// \file main.cpp

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>

#include <errno.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <sys/resource.h>
#include <sys/time.h>
#include <syslog.h>
#endif

#include <signal.h>

#include "pstore/database.hpp"
#include "pstore/shared_memory.hpp"
#include "pstore/time.hpp"
#include "pstore/vacuum_intf.hpp"
#include "pstore_support/error.hpp"
#include "pstore_support/logging.hpp"
#include "pstore_support/path.hpp"
#include "pstore_support/portab.hpp"
#include "pstore_support/quoted_string.hpp"
#include "pstore_support/thread.hpp"
#include "pstore_support/utf.hpp"

//#include "config.hpp"
#include "vacuum/copy.hpp"
#include "vacuum/quit.hpp"
#include "vacuum/status.hpp"
#include "vacuum/user_options.hpp"
#include "vacuum/watch.hpp"

#include "./switches.hpp"

#ifdef _WIN32
#include <tchar.h>
#define NATIVE_TEXT(str) _TEXT(str)
#else
typedef char TCHAR;
#define NATIVE_TEXT(str) str
#endif


#ifdef PSTORE_CPP_EXCEPTIONS
#define TRY try
#define CATCH(ex,code)  catch (ex) code
#else
#define TRY
#define CATCH(ex,code)
#endif


namespace {
#if defined(_WIN32) && defined(_UNICODE)
    auto & error_stream = std::wcerr;
#else
    auto & error_stream = std::cerr;
#endif
}


#if defined(_WIN32) && !defined(PSTORE_IS_INSIDE_LLVM)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;
    TRY {
        pstore::threads::set_name ("main");
        pstore::logging::create_log_stream ("vacuumd");

        vacuum::user_options user_opt;
        std::tie (user_opt, exit_code) = get_switches (argc, argv);
        if (exit_code != EXIT_SUCCESS) {
            return exit_code;
        }

#if 0
        if (user_opt.daemon_mode) {
#ifdef _WIN32
            if (::SetPriorityClass (::GetCurrentProcess (), PROCESS_MODE_BACKGROUND_BEGIN) == 0) {
                throw pstore::win32::exception ("SetPriorityClass", ::GetLastError ());
            }
#else
            // Increase our "niceness" value to reduce the priority of the process.
            if (::setpriority (PRIO_PROCESS, 0, 7) == -1) {
                raise (pstore::errno_erc {errno}, "setpriority");
            }
            if (daemon (0 /*nochdir*/, 0 /*noclose*/) != 0) {
                raise (pstore::errno_erc {errno}, "daemon");
            }
#endif
        }
#endif

        // bool const verbose = (options [verbose_opt] != nullptr);
        std::string const src_path = user_opt.src_path;
        std::string const src_dir = pstore::path::dir_name (src_path);

        pstore::logging::log (pstore::logging::priority::notice, "Start ",
                              pstore::quoted (src_path));

        // Superficially, we shouldn't need write access to the data store, but we do so because
        // once the collection is complete, we'll rename the temporary file that has been created
        // to the real file name that we're replacing. If the target file isn't writable, we
        // shouldn't try to replace it with the newer version.
        auto src_db = std::make_shared<pstore::database> (src_path, true /*writable*/,
                                                          false /*access tick enabled*/);

        vacuum::status st;
        std::thread quit_th = create_quit_thread (st, src_db);

        std::unique_lock<pstore::file::range_lock> * file_lock = src_db->upgrade_to_write_lock ();
        if (file_lock == nullptr) {
            assert (0);
        }

        if (file_lock->try_lock ()) {
            pstore::logging::log (pstore::logging::priority::info,
                                  "Got the file lock. No-one has the file open.");
            file_lock->unlock ();
        }

        std::thread copy_th (vacuum::copy, src_db, &st, std::ref (user_opt));
        std::thread watch_th (vacuum::watch, src_db, std::ref (*file_lock), &st);

        src_db.reset (); // main thread releases its reference to the source database.

        copy_th.join ();
        watch_th.join ();

        // We're done. Ask the quit thread to exit.
        notify_quit_thread ();
        quit_th.join ();

        // TODO:
        // On macOS we can do better than rename() [see man 2 exchangedata].
        // Similar elsewhere?

        pstore::logging::log (pstore::logging::priority::notice, "main () exiting: ",
                              pstore::quoted (src_path));
    } CATCH (std::exception const & ex, {
        char const * what = ex.what ();
        error_stream << NATIVE_TEXT ("vacuumd: An error occurred: ")
                     << pstore::utf::to_native_string (what) << std::endl;
        pstore::logging::log (pstore::logging::priority::error, "An error occurred: ", what);
        exit_code = EXIT_FAILURE;
    }) CATCH (..., {
        std::cerr << "vacuumd: An unknown error occurred." << std::endl;
        pstore::logging::log (pstore::logging::priority::error, "Unknown error");
        exit_code = EXIT_FAILURE;
    })

    return exit_code;
}
// eof: tools/vacuum/main.cpp
