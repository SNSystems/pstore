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
    class signal_cv {
    public:
        signal_cv ();

        // No copying or assignment.
        signal_cv (signal_cv const &) = delete;
        signal_cv & operator= (signal_cv const &) = delete;

        /// Blocks forever or until notify() is called by another thread or signal handler.
        void wait ();

        /// Releases lock and blocks the current executing thread. The thread will be unblocked when
        /// notify() is executed. When unblocked, lock is reacquired and wait exits. If this
        /// function exits via exception, lock is also reacquired. (until C++14)
        /// \param lock  An object of type std::unique_lock<std::mutex>, which must be locked by the
        /// current thread
        void wait (std::unique_lock<std::mutex> & lock);

        /// \param signal  The signal number responsible for the "wake".
        /// \note On POSIX, this function is called from a signal handler. It must only call
        /// signal-safe functions.
        void notify (int signal) noexcept;

        int signal () const { return signal_.load (); }

#ifdef _WIN32
        HANDLE get () const;
#endif

    private:
#ifdef _WIN32
        pstore::broker::pipe_descriptor event_;
#else
        pstore::broker::pipe_descriptor read_fd_;
        pstore::broker::pipe_descriptor write_fd_;
        static void make_non_blocking (int fd);
#endif
        std::atomic<int> signal_{-1};
    };

    // wait
    // ~~~~
    inline void signal_cv::wait (std::unique_lock<std::mutex> & lock) {
        lock.unlock ();
        this->wait ();
        lock.lock ();
    }

} // namespace pstore

#endif // PSTORE_SIGNAL_CV_HPP
