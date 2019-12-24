//*      _                   _               *
//*  ___(_) __ _ _ __   __ _| |   _____   __ *
//* / __| |/ _` | '_ \ / _` | |  / __\ \ / / *
//* \__ \ | (_| | | | | (_| | | | (__ \ V /  *
//* |___/_|\__, |_| |_|\__,_|_|  \___| \_/   *
//*        |___/                             *
//===- include/pstore/broker_intf/signal_cv.hpp ---------------------------===//
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

/// \file signal_cv.hpp

#ifndef PSTORE_SIGNAL_CV_HPP
#define PSTORE_SIGNAL_CV_HPP

#include <atomic>
#include <mutex>
#include "pstore/broker_intf/descriptor.hpp"

namespace pstore {

    /// On POSIX, This class implements the "self pipe trick" to enable a signal handler to call
    /// notify() to wake up a thread waiting on wait(). On Windows, an Event object is used to
    /// provide similar condition-variable like behavior.
    class descriptor_condition_variable {
    public:
        descriptor_condition_variable ();
        // No copying or assignment.
        descriptor_condition_variable (descriptor_condition_variable const &) = delete;
        descriptor_condition_variable (descriptor_condition_variable &&) = delete;
        descriptor_condition_variable & operator= (descriptor_condition_variable const &) = delete;
        descriptor_condition_variable & operator= (descriptor_condition_variable &&) = delete;

        virtual ~descriptor_condition_variable () = default;

        /// \brief Unblocks all threads currently waiting for *this.
        void notify_all ();

        /// \brief Unblocks all threads currently waiting for *this.
        ///
        /// \note On POSIX, this function is called from a signal handler. It must only call
        /// signal-safe functions.
        void notify_all_no_except () noexcept;

        /// Releases lock and blocks the current executing thread. The thread will be unblocked when
        /// notify() is executed. When unblocked, lock is reacquired and wait exits.
        ///
        /// \param lock  An object of type std::unique_lock<std::mutex>, which must be locked by the
        /// current thread
        void wait (std::unique_lock<std::mutex> & lock);

        void wait ();

        broker::pipe_descriptor const & wait_descriptor () const noexcept;
        void reset ();

    private:
#ifdef _WIN32
        broker::pipe_descriptor event_;
#else
        broker::pipe_descriptor read_fd_;
        broker::pipe_descriptor write_fd_;
        static void make_non_blocking (int fd);
        static int write (int fd) noexcept;
#endif
    };


    class signal_cv {
    public:
        signal_cv () = default;

        // No copying or assignment.
        signal_cv (signal_cv const &) = delete;
        signal_cv (signal_cv &&) = delete;
        signal_cv & operator= (signal_cv const &) = delete;
        signal_cv & operator= (signal_cv &&) = delete;

        /// \brief Unblocks all threads currently waiting for *this.
        ///
        /// \param signal  The signal number responsible for the "wake".
        /// \note On POSIX, this function is called from a signal handler. It must only call
        /// signal-safe functions.
        void notify_all (int const signal) noexcept {
            signal_ = signal;
            cv_.notify_all_no_except ();
        }

        /// Releases lock and blocks the current executing thread. The thread will be unblocked when
        /// notify() is executed. When unblocked, lock is reacquired and wait exits.
        ///
        /// \param lock  An object of type std::unique_lock<std::mutex>, which must be locked by the
        /// current thread
        void wait (std::unique_lock<std::mutex> & lock) { cv_.wait (lock); }
        void wait () { cv_.wait (); }

        broker::pipe_descriptor const & wait_descriptor () const noexcept {
            return cv_.wait_descriptor ();
        }
        void reset () { cv_.reset (); }

        int signal () const { return signal_.load (); }

    private:
        std::atomic<int> signal_{-1};
        descriptor_condition_variable cv_;
    };

} // namespace pstore

#endif // PSTORE_SIGNAL_CV_HPP
