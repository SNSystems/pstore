//*      _                   _               *
//*  ___(_) __ _ _ __   __ _| |   _____   __ *
//* / __| |/ _` | '_ \ / _` | |  / __\ \ / / *
//* \__ \ | (_| | | | | (_| | | | (__ \ V /  *
//* |___/_|\__, |_| |_|\__,_|_|  \___| \_/   *
//*        |___/                             *
//===- lib/os/signal_cv_posix.cpp -----------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

/// \file signal_cv_posix.cpp

#include "pstore/os/signal_cv.hpp"

#ifndef _WIN32

#    include <cassert>
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
    descriptor_condition_variable::descriptor_condition_variable () {
        enum { read_index, write_index, last_index };
        int fds_[last_index] = {0};
        if (::pipe (&fds_[0]) == -1) {
            raise (errno_erc{errno}, "pipe");
        }

        read_fd_.reset (fds_[read_index]);
        write_fd_.reset (fds_[write_index]);
        PSTORE_ASSERT (read_fd_.valid ());
        PSTORE_ASSERT (write_fd_.valid ());

        // Make both pipe descriptors non-blocking.
        descriptor_condition_variable::make_non_blocking (read_fd_.native_handle ());
        descriptor_condition_variable::make_non_blocking (write_fd_.native_handle ());
    }

    // wait descriptor
    // ~~~~~~~~~~~~~~~
    pipe_descriptor const & descriptor_condition_variable::wait_descriptor () const noexcept {
        return read_fd_;
    }

    // write
    // ~~~~~
    int descriptor_condition_variable::write (int const fd) noexcept {
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

    // notify all
    // ~~~~~~~~~~
    // To wake up the listener, we just write a single character to the write file descriptor.
    void descriptor_condition_variable::notify_all () {
        int const err = descriptor_condition_variable::write (write_fd_.native_handle ());
        if (err < 0) {
            raise (errno_erc{err}, "write");
        }
    }

    // notify all no except
    // ~~~~~~~~~~~~~~~~~~~~
    // To wake up the listener, we just write a single character to the write file descriptor.
    // On POSIX, this function is called from a signal handler. It must only call
    // signal-safe functions.
    void descriptor_condition_variable::notify_all_no_except () noexcept {
        descriptor_condition_variable::write (write_fd_.native_handle ());
    }

    // make non blocking
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
