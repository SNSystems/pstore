//*              *
//*   __ _  ___  *
//*  / _` |/ __| *
//* | (_| | (__  *
//*  \__, |\___| *
//*  |___/       *
//===- lib/broker/gc_posix.cpp --------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file gc_posix.cpp
/// \brief Waits for garbage-collection processes to exit. On exit, a process is removed from the
/// gc_watch_thread::processes_ collection.

#include "pstore/broker/gc.hpp"

#ifndef _WIN32

#    include <cstring>
#    include <chrono>
#    include <condition_variable>
#    include <thread>

#    include <sys/wait.h>

#    include "pstore/os/logging.hpp"
#    include "pstore/os/signal_helpers.hpp"

namespace {

    using priority = pstore::logger::priority;

    // (Making 'status' const here would introduce warnings because of the way that WCOREDUMP is
    // implemented on some platforms.)
    char const * core_dump_string (int status) {
#    ifdef WCOREDUMP
        if (WCOREDUMP (status)) { //! OCLint(PH - OS-provided macro)
            return "(core file generated)";
        }
#    endif
        return "(no core file available)";
    }

    // (Making 'status' const here would introduce warnings because of the way that WIFEXITED and
    // friend are implemented on some platforms.)
    void pr_exit (pid_t const pid, int status) {
        log (priority::info, "GC process exited pid ", pid);
        if (WIFEXITED (status)) { //! OCLint(PH - OS-provided macro)
            log (priority::info, "Normal termination, exit status = ", WEXITSTATUS (status));
        } else if (WIFSIGNALED (status)) { //! OCLint(PH - OS-provided macro)
            log (priority::info, "Abormal termination, signal number ", WTERMSIG (status));
            log (priority::info, "  ", core_dump_string (status));
        } else if (WIFSTOPPED (status)) { //! OCLint(PH - OS-provided macro)
            log (priority::info, "Child stopped, signal number = ", WSTOPSIG (status));
        }
    }

    std::string string_error (int const errnum) {
        static constexpr std::size_t buffer_size = 256U;
        char errbuf[buffer_size];
        errbuf[0] = '\0';

        char * pbuf = errbuf;
        errno = 0;

        // There are two variants of strerror_r(). The POSIX version returns an int and the GNU
        // version returns 'char *'. When _GNU_SOURCE is defined, glibc provides the char * version.
        // The GNU function doesn't usually modify the provided buffer -- it only modifies it
        // sometimes. If you end up with the GNU version, you must use the return value of the
        // function as the string to print.
#    if defined(_GNU_SOURCE)
        pbuf = ::strerror_r (errnum, pbuf, buffer_size);
#    else
        int const r = ::strerror_r (errnum, pbuf, buffer_size);
        (void) r;
        // r is assigned to ensure that we are getting the strerror_r() that returns int
#    endif
        if (errno != 0) {
            return "[unable to get message]";
        }
        if (pbuf[0] == '\0') {
            return "[empty string returned]";
        }
        return pbuf;
    }

} // end anonymous namespace


namespace pstore {
    namespace broker {

        // kill
        // ~~~~
        void gc_watch_thread::kill (process_identifier const & pid) {
            log (priority::info, "sending SIGINT to ", pid);
            ::kill (pid, SIGINT);
        }

        // child_signal
        // ~~~~~~~~~~~~
        /// \note This function is called from a signal handler so must restrict itself to
        /// signal-safe functions.
        void gc_watch_thread::child_signal (int const sig) {
            errno_saver const old_errno; //! OCLint(PH - meant to be unused)
            getgc ().cv_.notify_all (sig);
        }

        // watcher
        // ~~~~~~~
        void gc_watch_thread::watcher () {
            log (priority::info, "starting gc process watch thread");

            // Installing a global signal handler here is really somewhat antisocial. It would
            // overwrite a similar handler from another part of the system and it continues to be
            // active once this function returns.
            //
            // However, I'm comfortable enough with it at the moment that a more complex solution
            // is unnessary.
            register_signal_handler (SIGCHLD, &child_signal);

            std::unique_lock<decltype (mut_)> lock (mut_);
            for (;;) {
                try {
                    log (priority::info, "waiting for a GC process to complete");
                    cv_.wait (lock);
                    // There are two reasons that we may have been woken:
                    // - One (or more) of our child processes may have exited.
                    // - The program is exiting.
                    if (done_) {
                        break;
                    }

                    // Loop to ensure that we query all processes that exited whilst we were
                    // waiting.
                    auto status = 0;
                    while (auto const pid = ::waitpid (-1, &status, WUNTRACED | WNOHANG)) {
                        if (pid == -1) {
                            int const err = errno;
                            // If the error was "no child processes", we shouldn't report it.
                            if (err != ECHILD) {
                                log (priority::error, "waitpid error: ", string_error (err));
                            }
                            break;
                        }

                        std::string const & db_path = processes_.getl (pid);
                        log (priority::info, "GC exited for ", db_path);
                        // TODO: push a message to a channel which will tell a listener that this GC
                        // process has exited (and why)

                        pr_exit (pid, status); // Log the process exit.

                        processes_.eraser (pid); // Forget about the process.
                    }
                } catch (std::exception const & ex) {
                    log (priority::error, "An error occurred: ", ex.what ());
                    // TODO: delay before restarting. Don't restart after e.g. bad_alloc?
                } catch (...) {
                    log (priority::error, "Unknown error");
                    // TODO: delay before restarting
                }
            }

            // Ask any child GC processes to quit.
            log (priority::info, "cleaning up");
            std::for_each (processes_.right_begin (), processes_.right_end (),
                           [this] (pid_t const pid) { this->kill (pid); });
        }

    } // end namespace broker
} // end namespace pstore

#endif // _!WIN32
