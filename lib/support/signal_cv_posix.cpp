//*      _                   _               *
//*  ___(_) __ _ _ __   __ _| |   _____   __ *
//* / __| |/ _` | '_ \ / _` | |  / __\ \ / / *
//* \__ \ | (_| | | | | (_| | | | (__ \ V /  *
//* |___/_|\__, |_| |_|\__,_|_|  \___| \_/   *
//*        |___/                             *
//===- lib/support/signal_cv_posix.cpp ------------------------------------===//
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

/// \file signal_cv_posix.cpp

#include "pstore/support/signal_cv.hpp"

#ifndef _WIN32

#include <cassert>
#include <fcntl.h>
#include <unistd.h>

#include "pstore/support/error.hpp"

namespace pstore {

    // (ctor)
    // ~~~~~~
    signal_cv::signal_cv ()
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
        make_non_blocking (read_fd_.get ());
        make_non_blocking (write_fd_.get ());
    }

    // wait
    // ~~~~
    void signal_cv::wait () {
        auto const read_fd = read_fd_.get ();
        fd_set readfds{};
        FD_ZERO (&readfds);
        // Add the read end of pipe to 'readfds'.
        FD_SET (read_fd, &readfds); // NOLINT
        int nfds = read_fd + 1;
        do {
            int err = 0;
            while ((err = ::select (nfds, &readfds, nullptr, nullptr, nullptr)) == -1 &&
                   errno == EINTR) {
                continue; // Restart if interrupted by signal.
            }
            if (err == -1) {
                raise (errno_erc{errno}, "select");
            }

            char buffer{};
            ssize_t bytes_read = ::read (read_fd, &buffer, sizeof (buffer));
            if (bytes_read == -1) {
                raise (errno_erc{errno}, "read");
            }
        } while (!FD_ISSET (read_fd, &readfds));
    }

    // notify
    // ~~~~~~
    /// To wake up the listener, we just write a single characater to the write file descriptor.
    void signal_cv::notify (int signal) noexcept {
        signal_ = signal;
        auto const write_fd = write_fd_.get ();
        char const buffer = 'x';
        if (::write (write_fd, &buffer, sizeof (buffer)) == -1 && errno != EAGAIN) {
            ; // TODO: Can I report this error somehow?
        }
    }

    // make_non_blocking
    // ~~~~~~~~~~~~~~~~~
    void signal_cv::make_non_blocking (int fd) {
        int flags = ::fcntl (fd, F_GETFL); // NOLINT
        if (flags == -1) {
            raise (errno_erc{errno}, "fcntl");
        }
        flags |= O_NONBLOCK;
        if (::fcntl (fd, F_SETFL, flags) == -1) { // NOLINT
            raise (errno_erc{errno}, "fcntl");
        }
    }

} // namespace pstore
#endif //!_WIN32
// eof: lib/support/signal_cv_posix.cpp
