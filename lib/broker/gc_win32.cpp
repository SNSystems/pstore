//*              *
//*   __ _  ___  *
//*  / _` |/ __| *
//* | (_| | (__  *
//*  \__, |\___| *
//*  |___/       *
//===- lib/broker/gc_win32.cpp --------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file gc_win32.cpp
/// \brief Waits for garbage-collection processes to exit. On exit, a process is removed from the
/// gc_watch_thread::processes_ collection.

#include "pstore/broker/gc.hpp"

#ifdef _WIN32

// Standard library
#    include <mutex>
#    include <vector>

// Platform includes
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>

// pstore includes
#    include "pstore/broker/bimap.hpp"
#    include "pstore/broker/pointer_compare.hpp"
#    include "pstore/broker/spawn.hpp"
#    include "pstore/os/logging.hpp"
#    include "pstore/os/signal_cv.hpp"
#    include "pstore/os/thread.hpp"

namespace {

    void pr_exit (HANDLE process) {
        DWORD const pid = ::GetProcessId (process);

        auto child_exit_code = DWORD{0};
        if (::GetExitCodeProcess (process, &child_exit_code) == 0) {
            raise (pstore::win32_erc (::GetLastError ()), "GetExitCodeProcess");
        }

        using priority = pstore::logger::priority;

        log (priority::info, "GC process exited pid ", pid);
        if ((child_exit_code == EXIT_SUCCESS) || (child_exit_code == EXIT_FAILURE)) {
            log (priority::info, "Normal termination, exit status = ", child_exit_code);
        } else {
            log (priority::error, "Abnormal termination, exit status = ", child_exit_code);
        }
    }

    // build_object_vector
    // ~~~~~~~~~~~~~~~~~~~
    /// The parameter list for WaitForMultipleObjects() needs us to pass an array containing the
    /// handles for which we are to wait. However, to give us reasonable lookup performance, we've
    /// got those handles in a bimap (bi-directional map). This function updates an existing vector
    /// of HANDLEs to reflect the current collections of processes recorded by that bimap. It also
    /// adds the handle associated with the "notify condition variable".
    ///
    /// \param processes  A bimap whose 'right' collection owns the process handles for all of the
    /// garbage-collection processes being managed.
    /// \param cv  A condition-variable-like object which is used to wake the thread in the
    /// event of a child process exiting.
    /// \param[out] v  A vector which will on return contain the process handles from the
    /// \p processes collection and the \p cv condition-variable. An existing contents are
    /// destroyed.
    template <typename ProcessBimap>
    void build_object_vector (ProcessBimap const & processes, pstore::signal_cv const & cv,
                              pstore::gsl::not_null<std::vector<HANDLE> *> v) {
        // The size of the vector is rounded up in order to reduce the probability of a memory
        // allocation being required when the vector is resized. In typical use, the number of
        // elements in the output vector will likely be small (less than round_to?)
        constexpr auto round_to = std::size_t{8};
        auto const roundup = [] (std::size_t v, std::size_t m) { return v + (m - v % m); };

        v->clear ();
        v->reserve (roundup (processes.size () + 1U, round_to));
        v->push_back (cv.wait_descriptor ().native_handle ());
        std::transform (
            processes.right_begin (), processes.right_end (), std::back_inserter (*v),
            [] (pstore::broker::process_identifier const & pid) { return pid->process (); });
    }

} // end anonymous namespace

namespace pstore {
    namespace broker {

        // kill
        // ~~~~
        void gc_watch_thread::kill (process_identifier const & pid) {
            log (logger::priority::info, "sending CTRL_BREAK_EVENT to ", pid->group ());
            if (!::GenerateConsoleCtrlEvent (CTRL_BREAK_EVENT, pid->group ())) {
                log (logger::priority::error, "An error occurred: ", ::GetLastError ());
            }
        }

        // watcher
        // ~~~~~~~
        void gc_watch_thread::watcher () {
            log (logger::priority::info, "starting gc process watch thread");

            std::vector<HANDLE> object_vector;
            std::unique_lock<decltype (mut_)> lock (mut_);

            for (;;) {
                try {
                    log (logger::priority::info, "waiting for a GC process to complete");

                    build_object_vector (processes_, cv_, &object_vector);
                    auto const num_objects = object_vector.size ();
                    PSTORE_ASSERT (num_objects > 0 && num_objects <= MAXIMUM_WAIT_OBJECTS);

                    lock.unlock ();
                    constexpr DWORD wmo_timeout = 60 * 1000; // 60 second timeout.
                    DWORD const wmo_res = ::WaitForMultipleObjects (
                        static_cast<DWORD> (num_objects), // Size of the handle array.
                        object_vector.data (),            // The array of handles on which to wait.
                        FALSE,                            // Wait for all handles?
                        wmo_timeout                       // Timeout (milliseconds)
                    );
                    DWORD const last_error = ::GetLastError ();
                    lock.lock ();

                    // We may have been woken up because the program is exiting.
                    if (done_) {
                        break;
                    }

                    if (wmo_res == WAIT_FAILED) {
                        raise (pstore::win32_erc{last_error}, "WaitForMultipleObjects failed");
                    } else if (wmo_res == WAIT_TIMEOUT) {
                        log (logger::priority::info, "WaitForMultipleObjects timeout");
                    } else if (wmo_res >= WAIT_OBJECT_0) {
                        // Extract the handle that caused us to wake.
                        HANDLE const h = object_vector.at (wmo_res - WAIT_OBJECT_0);
                        // We may have been woken by the notify condition variable rather than as a
                        // result of a process exiting.
                        if (h != cv_.wait_descriptor ().native_handle ()) {
                            // A GC process exited so let the user know and remove it from the
                            // collection of child processes.
                            pr_exit (h);
                            processes_.eraser (h);
                        }
                    } else if (wmo_res >= WAIT_ABANDONED_0) {
                        // "If a thread terminates without releasing its ownership of a mutex
                        // object, the mutex object is considered to be abandoned." We don't expect
                        // that to ever happen here.
                        log (logger::priority::error,
                             "WaitForMultipleObjects WAIT_ABANDONED error n=",
                             wmo_res - WAIT_ABANDONED_0);
                    } else {
                        log (logger::priority::error,
                             "Unknown WaitForMultipleObjects return value ", wmo_res);
                    }
                } catch (std::exception const & ex) {
                    log (logger::priority::error, "An error occurred: ", ex.what ());
                    // TODO: delay before restarting. Don't restart after e.g. bad_alloc?
                } catch (...) {
                    log (logger::priority::error, "Unknown error");
                    // TODO: delay before restarting
                }
            }

            // Tell any child GC processes to quit.
            log (logger::priority::info, "cleaning up");
            std::for_each (processes_.right_begin (), processes_.right_end (),
                           [this] (broker::process_identifier const & pid) { this->kill (pid); });
        }

    } // end namespace broker
} // end namespace pstore

#endif // _WIN32
