//*      _                   _               *
//*  ___(_) __ _ _ __   __ _| |   _____   __ *
//* / __| |/ _` | '_ \ / _` | |  / __\ \ / / *
//* \__ \ | (_| | | | | (_| | | | (__ \ V /  *
//* |___/_|\__, |_| |_|\__,_|_|  \___| \_/   *
//*        |___/                             *
//===- lib/broker_intf/signal_cv_posix.cpp --------------------------------===//
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

/// \file signal_cv_posix.cpp

#include "pstore/broker_intf/signal_cv.hpp"

#ifndef _WIN32

#    include <cassert>
#    include <cstring> // for memset() [used by FD_ZERO on solaris]
#    include <fcntl.h>
#    include <poll.h>
#    include <unistd.h>

#    include "pstore/support/error.hpp"
#    include "pstore/support/scope_guard.hpp"

namespace {

    /// Instances of this type are written to and read from our pipe.
    using pipe_content_type = char;

} // end anonymous namespace

namespace pstore {

    //*     _                _      _              _____   __ *
    //*  __| |___ ___ __ _ _(_)_ __| |_ ___ _ _   / __\ \ / / *
    //* / _` / -_|_-</ _| '_| | '_ \  _/ _ \ '_| | (__ \ V /  *
    //* \__,_\___/__/\__|_| |_| .__/\__\___/_|    \___| \_/   *
    //*                       |_|                             *
    // (ctor)
    // ~~~~~~
    descriptor_condition_variable::descriptor_condition_variable ()
            : read_fd_{}
            , write_fd_{} {

        enum { read_index, write_index, last_index };
        int fds_[last_index] = {0};
        if (::pipe (&fds_[0]) == -1) {
            raise (errno_erc{errno}, "pipe");
        }

        read_fd_.reset (fds_[read_index]);
        write_fd_.reset (fds_[write_index]);
        assert (read_fd_.valid ());
        assert (write_fd_.valid ());

        // Make both pipe descriptors non-blocking.
        descriptor_condition_variable::make_non_blocking (read_fd_.native_handle ());
        descriptor_condition_variable::make_non_blocking (write_fd_.native_handle ());
    }

    // wait_descriptor
    // ~~~~~~~~~~~~~~~
    pstore::broker::pipe_descriptor const & descriptor_condition_variable::wait_descriptor () const
        noexcept {
        return read_fd_;
    }

    // write
    // ~~~~~
    int descriptor_condition_variable::write (int fd) noexcept {
        pipe_content_type const buffer{};
        for (;;) {
            if (::write (fd, &buffer, sizeof (buffer)) < 0) {
                if (errno != EINTR && errno != EAGAIN) {
                    return errno;
                }
                // Retry the write.
            } else {
                // Success.
                break;
            }
        }
        return 0;
    }

    // notify_all
    // ~~~~~~~~~~
    // To wake up the listener, we just write a single character to the write file descriptor.
    void descriptor_condition_variable::notify_all () {
        int const err = descriptor_condition_variable::write (write_fd_.native_handle ());
        if (err < 0) {
            raise (errno_erc{err}, "write");
        }
    }

    // notify_all_no_except
    // ~~~~~~~~~~~~~~~~~~~~
    // To wake up the listener, we just write a single character to the write file descriptor.
    // On POSIX, this function is called from a signal handler. It must only call
    // signal-safe functions.
    void descriptor_condition_variable::notify_all_no_except () noexcept {
        descriptor_condition_variable::write (write_fd_.native_handle ());
    }

    // make_non_blocking
    // ~~~~~~~~~~~~~~~~~
    void descriptor_condition_variable::make_non_blocking (int const fd) {
        int flags = ::fcntl (fd, F_GETFL); // NOLINT
        if (flags == -1) {
            raise (errno_erc{errno}, "fcntl");
        }
        flags |= O_NONBLOCK;
        if (::fcntl (fd, F_SETFL, flags) == -1) { // NOLINT
            raise (errno_erc{errno}, "fcntl");
        }
    }

    // wait
    // ~~~~
    void descriptor_condition_variable::wait () {
        int const read_fd = this->wait_descriptor ().native_handle ();
        auto const pollin = [] (pollfd const & pfd) { return pfd.revents & POLLIN; };
        pollfd pollfds = {read_fd, POLLIN, 0};
        int count = 0;
        do {
            errno = 0;
            constexpr int timeout = -1; // Wait forever.
            while ((count = ::poll (&pollfds, 1, timeout)) == -1 && errno == EINTR) {
                errno = 0; // Restart if interrupted by signal.
            }
            if (count == -1) {
                raise (errno_erc{errno}, "select");
            }

            this->reset ();
        } while (count != 1 && !pollin (pollfds));
    }

    void descriptor_condition_variable::wait (std::unique_lock<std::mutex> & lock) {
        lock.unlock ();
        auto const _ = //! OCLINT(PH - variable is intentionally unused)
            make_scope_guard ([&lock] () { lock.lock (); });
        this->wait ();
    }

    // reset
    // ~~~~~
    void descriptor_condition_variable::reset () {
        pipe_content_type buffer{};
        ssize_t const bytes_read =
            ::read (this->wait_descriptor ().native_handle (), &buffer, sizeof (buffer));
        if (bytes_read == -1) {
            raise (errno_erc{errno}, "read");
        }
    }

} // namespace pstore
#endif //!_WIN32
