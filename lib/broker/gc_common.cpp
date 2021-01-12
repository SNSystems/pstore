//*                                                          *
//*   __ _  ___    ___ ___  _ __ ___  _ __ ___   ___  _ __   *
//*  / _` |/ __|  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
//* | (_| | (__  | (_| (_) | | | | | | | | | | | (_) | | | | *
//*  \__, |\___|  \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
//*  |___/                                                   *
//===- lib/broker/gc_common.cpp -------------------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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

#include "pstore/os/logging.hpp"
#include "pstore/os/path.hpp"
#include "pstore/os/process_file_name.hpp"
#include "pstore/os/signal_helpers.hpp"

namespace pstore {
    namespace broker {

        using priority = logger::priority;

        gc_watch_thread::~gc_watch_thread () noexcept = default;

        // spawn
        // ~~~~~
        process_identifier gc_watch_thread::spawn (std::initializer_list<gsl::czstring> argv) {
            return broker::spawn (argv);
        }

        // start_vacuum
        // ~~~~~~~~~~~~
        void gc_watch_thread::start_vacuum (std::string const & db_path) {
            std::unique_lock<decltype (mut_)> const lock{mut_};
            if (processes_.presentl (db_path)) {
                log (priority::info, "GC process is already running for ",
                     logger::quoted{db_path.c_str ()});
                return;
            }

            if (processes_.size () >= max_gc_processes) {
                log (priority::info,
                     "Maximum number of GC processes are running. Ignoring request for ",
                     logger::quoted{db_path.c_str ()});
                return;
            }
            log (priority::info, "Starting GC process for ", logger::quoted{db_path.c_str ()});
            processes_.set (db_path,
                            this->spawn ({vacuumd_path ().c_str (), db_path.c_str (), nullptr}));

            // An initial wakeup of the GC-watcher thread in case the child process exited
            // before we had time to install the SIGCHLD signal handler.
            cv_.notify_all (-1);
        }

        // get_pid
        // ~~~~~~~
        maybe<process_identifier> gc_watch_thread::get_pid (std::string const & path) {
            std::unique_lock<decltype (mut_)> const lock{mut_};
            if (!processes_.presentl (path)) {
                return {};
            }
            return maybe<process_identifier>{processes_.getr (path)};
        }

        // stop_vacuum
        // ~~~~~~~~~~~
        bool gc_watch_thread::stop_vacuum (std::string const & path) {
            if (maybe<process_identifier> const pid = this->get_pid (path)) {
                log (priority::info, "Killing GC for ", logger::quoted{path.c_str ()});
                this->kill (*pid);
                processes_.erasel (path);
                return true;
            }
            log (priority::info, "No GC process running for ", logger::quoted{path.c_str ()});
            return false;
        }

        // stop
        // ~~~~
        void gc_watch_thread::stop (int const signum) {
            {
                std::unique_lock<decltype (mut_)> const lock{mut_};
                done_ = true;
            }
            log (priority::info, "asking gc process watch thread to exit");
            cv_.notify_all (signum);
        }

        // size
        // ~~~~
        std::size_t gc_watch_thread::size () const {
            std::unique_lock<decltype (mut_)> const lock{mut_};
            return processes_.size ();
        }

        // vacuumd_path
        // ~~~~~~~~~~~~
        std::string gc_watch_thread::vacuumd_path () {
            using path::dir_name;
            using path::join;
            return join (dir_name (process_file_name ()), vacuumd_name);
        }


        /// Manages the sole local gc_watch_thread
        gc_watch_thread & getgc () {
            static pstore::broker::gc_watch_thread gc;
            return gc;
        }

        void gc_process_watch_thread () { getgc ().watcher (); }

        void start_vacuum (std::string const & db_path) { getgc ().start_vacuum (db_path); }

        /// Called when a signal has been recieved which should result in the process shutting.
        /// \note This function is called from the quit-thread rather than directly from a signal
        /// handler so it doesn't need to restrict itself to signal-safe functions.
        void gc_sigint (int const sig = -1) { getgc ().stop (sig); }

    } // end namespace broker
} // end namespace pstore
