//*                                                          *
//*   __ _  ___    ___ ___  _ __ ___  _ __ ___   ___  _ __   *
//*  / _` |/ __|  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
//* | (_| | (__  | (_| (_) | | | | | | | | | | | (_) | | | | *
//*  \__, |\___|  \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
//*  |___/                                                   *
//===- lib/broker/gc_common.cpp -------------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
/// \file gc_common.cpp

#include "pstore/broker/gc.hpp"

#ifndef _WIN32
#include <signal.h>
#endif

#include "pstore/broker/globals.hpp"
#include "pstore/broker/spawn.hpp"
#include "pstore/support/logging.hpp"
#include "pstore/support/path.hpp"
#include "pstore/support/process_file_name.hpp"
#include "pstore/support/signal_helpers.hpp"

namespace pstore {
    namespace broker {

        // start_vacuum
        // ~~~~~~~~~~~~
        void gc_watch_thread::start_vacuum (std::string const & db_path) {
            std::unique_lock<decltype (mut_)> lock (mut_);
            if (processes_.presentl (db_path)) {
                logging::log (logging::priority::info, "GC process is already running for ",
                              logging::quoted (db_path.c_str ()));
            } else {
                logging::log (logging::priority::info, "Starting GC process for ",
                              logging::quoted{db_path.c_str ()});
                auto const exe_path = vacuumd_path ();
                std::array<char const *, 4> argv = {
                    {exe_path.c_str (), "--daemon", db_path.c_str (), nullptr}};
                auto child_identifier = spawn (exe_path.c_str (), argv.data ());
                processes_.set (db_path, child_identifier);

                // An initial wakeup of the GC-watcher thread in case the child process exited
                // before we
                // had time to install the SIGCHLD signal handler.
                cv_.notify (-1);
            }
        }

        // stop
        // ~~~~
        void gc_watch_thread::stop (int signum) {
            assert (done);
            logging::log (logging::priority::info, "asking gc process watch thread to exit");
            cv_.notify (signum);
        }

        // vacuumd_path
        // ~~~~~~~~~~~~
        std::string gc_watch_thread::vacuumd_path () {
            using path::dir_name;
            using path::join;
            return join (dir_name (process_file_name ()), vacuumd_name);
        }

    } // end namespace broker
} // end namespace pstore

namespace {

    /// Manages the sole local gc_watch_thread
    pstore::broker::gc_watch_thread & getgc () {
        static pstore::broker::gc_watch_thread gc;
        return gc;
    }

#ifndef _WIN32

    // child_signal
    // ~~~~~~~~~~~~
    void child_signal (int sig) {
        pstore::errno_saver old_errno;
        getgc ().child_signal (sig);
    }

#endif // !_WIN32

} // end anonymous namespace


namespace pstore {
    namespace broker {

        void gc_process_watch_thread () {
#ifndef _WIN32
            register_signal_handler (SIGCHLD, child_signal);
#endif
            getgc ().watcher ();
        }

        void start_vacuum (std::string const & db_path) { getgc ().start_vacuum (db_path); }

        /// Called when a signal has been recieved which should result in the process shutting.
        /// \note This function is called from the quit-thread rather than directly from a signal
        /// handler so it doesn't need to restrict itself to signal-safe functions.
        void gc_sigint (int sig) { getgc ().stop (sig); }

    } // end namespace broker
} // end namespace pstore

// eof: lib/broker/gc_common.cpp
