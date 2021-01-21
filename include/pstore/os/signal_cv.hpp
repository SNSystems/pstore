//*      _                   _               *
//*  ___(_) __ _ _ __   __ _| |   _____   __ *
//* / __| |/ _` | '_ \ / _` | |  / __\ \ / / *
//* \__ \ | (_| | | | | (_| | | | (__ \ V /  *
//* |___/_|\__, |_| |_|\__,_|_|  \___| \_/   *
//*        |___/                             *
//===- include/pstore/os/signal_cv.hpp ------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

/// \file signal_cv.hpp

#ifndef PSTORE_OS_SIGNAL_CV_HPP
#define PSTORE_OS_SIGNAL_CV_HPP

#include <atomic>
#include <mutex>

#include "pstore/os/descriptor.hpp"

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

        /// Releases lock and blocks the current executing thread. The thread will be unblocked
        /// when notify() is executed. When unblocked, lock is reacquired and wait exits.
        ///
        /// \param lock  An object of type std::unique_lock<std::mutex>, which must be locked by
        /// the current thread
        void wait (std::unique_lock<std::mutex> & lock);

        void wait ();

        pipe_descriptor const & wait_descriptor () const noexcept;
        void reset ();

    private:
#ifdef _WIN32
        pipe_descriptor event_;
#else
        pipe_descriptor read_fd_{};
        pipe_descriptor write_fd_{};

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

        ~signal_cv () noexcept = default;

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

        /// Releases lock and blocks the current executing thread. The thread will be unblocked
        /// when notify() is executed. When unblocked, lock is reacquired and wait exits.
        ///
        /// \param lock  An object of type std::unique_lock<std::mutex>, which must be locked by
        /// the current thread
        void wait (std::unique_lock<std::mutex> & lock) { cv_.wait (lock); }
        void wait () { cv_.wait (); }

        pipe_descriptor const & wait_descriptor () const noexcept { return cv_.wait_descriptor (); }
        void reset () { cv_.reset (); }

        int signal () const { return signal_.load (); }

    private:
        std::atomic<int> signal_{-1};
        descriptor_condition_variable cv_;
    };

} // end namespace pstore

#endif // PSTORE_OS_SIGNAL_CV_HPP
