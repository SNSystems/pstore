//*              _ _    *
//*   __ _ _   _(_) |_  *
//*  / _` | | | | | __| *
//* | (_| | |_| | | |_  *
//*  \__, |\__,_|_|\__| *
//*     |_|             *
//===- lib/vacuum/quit.cpp ------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file vacuum/quit.cpp

#include "pstore/vacuum/quit.hpp"

// Standard includes
#include <atomic>
#include <cassert>
#include <cerrno>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>

// Platform includes
#include <fcntl.h>
#ifndef _WIN32
#    include <unistd.h>
#endif
#include <signal.h>

#include "pstore/core/database.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/os/signal_cv.hpp"
#include "pstore/os/signal_helpers.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/vacuum/status.hpp"
#include "pstore/vacuum/watch.hpp"

namespace {

    pstore::signal_cv quit_info;

    //***************
    //* quit thread *
    //***************
    void quit_thread (vacuum::status & status, std::weak_ptr<pstore::database> const /*src_db*/) {
        using priority = pstore::logger::priority;
        PSTORE_TRY {
            pstore::threads::set_name ("quit");
            pstore::create_log_stream ("vacuum.quit");

            // Wait for to be told that we are in the process of shutting down. This
            // call will block until signal_handler() [below] is called by, for example,
            // the user typing Control-C.
            quit_info.wait ();

            log (priority::info, "Signal received. Will terminate after current command. Num=",
                 quit_info.signal ());
            status.done = true;

            // Wake up the watch thread immediately.
            vacuum::wst.start_watch_cv.notify_one ();
            // vacuum::wst.complete_cv.notify_all ();
            log (priority::notice, "Marked job as done. Notified start_watch_cv and complete_cv.");
        }
        // clang-format off
        PSTORE_CATCH (std::exception const & ex, { // clang-format on
            log (priority::error, "error:", ex.what ());
        })
        // clang-format off
        PSTORE_CATCH (..., { // clang-format on
            log (priority::error, "unknown exception");
        })
        // clang-format on
    }


    // signal_handler
    // ~~~~~~~~~~~~~~
    /// A signal handler entry point.
    void signal_handler (int const sig) {
        pstore::errno_saver const saver;
        quit_info.notify_all (sig);
    }

} // end anonymous namespace


// notify_quit_thread
// ~~~~~~~~~~~~~~~~~~
void notify_quit_thread () {
    quit_info.notify_all (-1);
}

// create_quit_thread
// ~~~~~~~~~~~~~~~~~~
std::thread create_quit_thread (vacuum::status & status,
                                std::shared_ptr<pstore::database> const & src_db) {
    std::weak_ptr<pstore::database> db{src_db};
    std::thread quit (quit_thread, std::ref (status), db);
    pstore::register_signal_handler (SIGINT, signal_handler);
    pstore::register_signal_handler (SIGTERM, signal_handler);
#ifdef _WIN32
    pstore::register_signal_handler (SIGBREAK, signal_handler); // Ctrl-Break sequence
#endif
    return quit;
}
