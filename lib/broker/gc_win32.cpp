//*              *
//*   __ _  ___  *
//*  / _` |/ __| *
//* | (_| | (__  *
//*  \__, |\___| *
//*  |___/       *
//===- lib/broker/gc_win32.cpp --------------------------------------------===//
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
#    include "pstore/broker/globals.hpp"
#    include "pstore/broker/pointer_compare.hpp"
#    include "pstore/broker/spawn.hpp"
#    include "pstore/broker_intf/signal_cv.hpp"
#    include "pstore/os/logging.hpp"
#    include "pstore/os/thread.hpp"


namespace {

    void pr_exit (HANDLE process) {
        DWORD const pid = ::GetProcessId (process);

        auto child_exit_code = DWORD{0};
        if (::GetExitCodeProcess (process, &child_exit_code) == 0) {
            raise (pstore::win32_erc (::GetLastError ()), "GetExitCodeProcess");
        }

        using pstore::logging::priority;
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
        auto const roundup = [](std::size_t v, std::size_t m) { return v + (m - v % m); };

        v->clear ();
        v->reserve (roundup (processes.size () + 1U, round_to));
        v->push_back (cv.wait_descriptor ().native_handle ());
        std::transform (
            processes.right_begin (), processes.right_end (), std::back_inserter (*v),
            [](pstore::broker::process_identifier const & process) { return process.get (); });
    }

} // end anonymous namespace

namespace pstore {
    namespace broker {

        // watcher
        // ~~~~~~~
        void gc_watch_thread::watcher () {
            using logging::priority;
            log (priority::info, "starting gc process watch thread");

            std::vector<HANDLE> object_vector;
            std::unique_lock<decltype (mut_)> lock (mut_);

            while (!done) {
                try {
                    log (priority::info, "waiting for a GC process to complete");

                    build_object_vector (processes_, cv_, &object_vector);
                    // FIXME: handle the (highly unlikely) case where this assertion would fire.
                    assert (object_vector.size () < MAXIMUM_WAIT_OBJECTS);

                    lock.unlock ();
                    DWORD const wmo_timeout = 60 * 1000; // 60 second timeout.
                    DWORD wmo_res = ::WaitForMultipleObjects (
                        static_cast<DWORD> (object_vector.size ()), object_vector.data (),
                        FALSE /*wait all*/, wmo_timeout);
                    lock.lock ();

                    // We may have been woken up because the program is exiting.
                    if (done) {
                        continue;
                    }

                    if (wmo_res == WAIT_FAILED) {
                        auto errcode = ::GetLastError ();
                        log (priority::error, "WaitForMultipleObjects error ", errcode); // FIXME: exception
                    } else if (wmo_res == WAIT_TIMEOUT) {
                        log (priority::info, "WaitForMultipleObjects timeout");
                    } else if (wmo_res >= WAIT_OBJECT_0) {
                        // Extract the handle that caused us to wake.
                        HANDLE h = object_vector.at (wmo_res - WAIT_OBJECT_0);
                        // We may have been woken by the notify condition variable rather than as a
                        // result of a process exiting.
                        if (h != cv_.wait_descriptor ().native_handle ()) {
                            // A GC process exited so let the user know and remove it from the
                            // collection of child processes.
                            pr_exit (h);
                            processes_.eraser (h);
                        }
                    } else {
                        // TODO: more conditions here?
                    }
                } catch (std::exception const & ex) {
                    log (priority::error, "An error occurred: ", ex.what ());
                    // TODO: delay before restarting. Don't restart after e.g. bad_alloc?
                } catch (...) {
                    log (priority::error, "Unknown error");
                    // TODO: delay before restarting
                }
            }

            // Tell any child GC processes to quit.
            log (priority::info, "cleaning up");
            auto const kill = [](broker::process_identifier const & pid) {
                log (priority::info, "sending CTRL_BREAK_EVENT to ", pid->group ());
                if (!::GenerateConsoleCtrlEvent (CTRL_BREAK_EVENT, pid->group ())) {
                    log (priority::error, "An error occurred: ", ::GetLastError ());
                }
            };
            std::for_each (processes_.right_begin (), processes_.right_end (), kill);
        }

    } // end namespace broker
} // end namespace pstore

#endif // _WIN32
