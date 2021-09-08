//===- tools/vacuum/main.cpp ----------------------------------------------===//
//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#else
#    include <sys/resource.h>
#    include <sys/time.h>
#    include <syslog.h>
#endif

#include <signal.h>

#include "pstore/core/database.hpp"
#include "pstore/core/time.hpp"
#include "pstore/core/vacuum_intf.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/os/path.hpp"
#include "pstore/os/shared_memory.hpp"
#include "pstore/os/thread.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp"
#include "pstore/vacuum/copy.hpp"
#include "pstore/vacuum/quit.hpp"
#include "pstore/vacuum/status.hpp"
#include "pstore/vacuum/user_options.hpp"
#include "pstore/vacuum/watch.hpp"

#include "./switches.hpp"

namespace {

#ifdef PSTORE_EXCEPTIONS
    auto & error_stream =
#    if defined(_WIN32) && defined(_UNICODE)
        std::wcerr;
#    else
        std::cerr;
#    endif
#endif // PSTORE_EXCEPTIONS

} // namespace


#if defined(_WIN32)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;
    using priority = pstore::logger::priority;
    PSTORE_TRY {
        pstore::threads::set_name ("main");
        pstore::create_log_stream ("vacuumd");

        vacuum::user_options user_opt;
        std::tie (user_opt, exit_code) = get_switches (argc, argv);
        if (exit_code != EXIT_SUCCESS) {
            return exit_code;
        }

#if 0
        if (user_opt.daemon_mode) {
#    ifdef _WIN32
            if (::SetPriorityClass (::GetCurrentProcess (), PROCESS_MODE_BACKGROUND_BEGIN) == 0) {
                throw pstore::win32::exception ("SetPriorityClass", ::GetLastError ());
            }
#    else
            // Increase our "niceness" value to reduce the priority of the process.
            if (::setpriority (PRIO_PROCESS, 0, 7) == -1) {
                raise (pstore::errno_erc {errno}, "setpriority");
            }
            if (daemon (0 /*nochdir*/, 0 /*noclose*/) != 0) {
                raise (pstore::errno_erc {errno}, "daemon");
            }
#    endif
        }
#endif

        // bool const verbose = (options [verbose_opt] != nullptr);
        std::string const src_path = user_opt.src_path;
        std::string const src_dir = pstore::path::dir_name (src_path);

        log (priority::notice, "Start ", pstore::logger::quoted{src_path.c_str ()});

        // Superficially, we shouldn't need write access to the data store, but we do so because
        // once the collection is complete, we'll rename the temporary file that has been created
        // to the real file name that we're replacing. If the target file isn't writable, we
        // shouldn't try to replace it with the newer version.
        auto src_db = std::make_shared<pstore::database> (
            src_path, pstore::database::access_mode::writable, false /*access tick enabled*/);

        vacuum::status st;
        std::thread quit_th = create_quit_thread (st, src_db);

        std::unique_lock<pstore::file::range_lock> * file_lock = src_db->upgrade_to_write_lock ();
        if (file_lock == nullptr) {
            PSTORE_ASSERT (0);
        }

        if (file_lock->try_lock ()) {
            log (priority::info, "Got the file lock. No-one has the file open.");
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

        log (priority::notice, "main () exiting: ", pstore::logger::quoted{src_path.c_str ()});
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, { // clang-format on
        char const * what = ex.what ();
        error_stream << PSTORE_NATIVE_TEXT ("vacuumd: An error occurred: ")
                     << pstore::utf::to_native_string (what) << std::endl;
        log (priority::error, "An error occurred: ", what);
        exit_code = EXIT_FAILURE;
    })
    // clang-format off
    PSTORE_CATCH (..., { // clang-format on
        std::cerr << "vacuumd: An unknown error occurred." << std::endl;
        log (priority::error, "Unknown error");
        exit_code = EXIT_FAILURE;
    })

    return exit_code;
}
