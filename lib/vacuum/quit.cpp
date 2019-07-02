//*              _ _    *
//*   __ _ _   _(_) |_  *
//*  / _` | | | | | __| *
//* | (_| | |_| | | |_  *
//*  \__, |\__,_|_|\__| *
//*     |_|             *
//===- lib/vacuum/quit.cpp ------------------------------------------------===//
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
#include <unistd.h>
#endif
#include <signal.h>

#include "pstore/broker_intf/signal_cv.hpp"
#include "pstore/core/database.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/signal_helpers.hpp"
#include "pstore/vacuum/status.hpp"
#include "pstore/vacuum/watch.hpp"

namespace {

    pstore::signal_cv quit_info;

    //***************
    //* quit thread *
    //***************
    void quit_thread (vacuum::status & status, std::weak_ptr<pstore::database> /*src_db*/) {
        PSTORE_TRY {
            pstore::threads::set_name ("quit");
            pstore::logging::create_log_stream ("vacuum.quit");

            // Wait for to be told that we are in the process of shutting down. This
            // call will block until signal_handler() [below] is called by, for example,
            // the user typing Control-C.
            quit_info.wait ();

            pstore::logging::log (
                pstore::logging::priority::info,
                "Signal received. Will terminate after current command. Num=", quit_info.signal ());
            status.done = true;

            // Wake up the watch thread immediately.
            vacuum::wst.start_watch_cv.notify_one ();
            // vacuum::wst.complete_cv.notify_all ();
            pstore::logging::log (pstore::logging::priority::notice,
                                  "Marked job as done. Notified start_watch_cv and complete_cv.");
        }
        // clang-format off
        PSTORE_CATCH (std::exception const & ex, {
            pstore::logging::log (pstore::logging::priority::error, "error:", ex.what ());
        })
        PSTORE_CATCH (..., {
            pstore::logging::log (pstore::logging::priority::error, "unknown exception");
        })
        // clang-format on
    }


    // signal_handler
    // ~~~~~~~~~~~~~~
    /// A signal handler entry point.
    void signal_handler (int sig) {
        pstore::errno_saver saver;
        quit_info.notify_all (sig);
    }

} // namespace


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
