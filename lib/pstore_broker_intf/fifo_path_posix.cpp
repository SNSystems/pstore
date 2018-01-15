//*   __ _  __                     _   _      *
//*  / _(_)/ _| ___    _ __   __ _| |_| |__   *
//* | |_| | |_ / _ \  | '_ \ / _` | __| '_ \  *
//* |  _| |  _| (_) | | |_) | (_| | |_| | | | *
//* |_| |_|_|  \___/  | .__/ \__,_|\__|_| |_| *
//*                   |_|                     *
//===- lib/pstore_broker_intf/fifo_path_posix.cpp -------------------------===//
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
/// \file fifo_path_posix.cpp
/// \brief Portions of the implementation of the broker::fifo_path class that are POSIX-specific.

#include "pstore_broker_intf/fifo_path.hpp"

#include <sstream>
#include <thread>

#ifndef _WIN32

#include <cerrno>
#include <cstdlib>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pstore_support/error.hpp"

namespace {

    class umask_raii {
    public:
        explicit umask_raii (mode_t new_umask)
                : old_{::umask (new_umask)} {}
        umask_raii (umask_raii &&) = delete;
        umask_raii (umask_raii const &) = delete;
        umask_raii & operator= (umask_raii const &) = delete;
        umask_raii & operator= (umask_raii &&) = delete;
        ~umask_raii () {
            ::umask (old_);
        }

    private:
        mode_t const old_;
    };

} // (anonymous namespace)

namespace pstore {
    namespace broker {

        // (dtor)
        // ~~~~~~
        fifo_path::~fifo_path () {
            bool expected = true;
            if (needs_delete_.compare_exchange_strong (expected, false)) {
                ::unlink (path_.c_str ());
            }
        }

        // open_server_pipe
        // ~~~~~~~~~~~~~~~~
        auto fifo_path::open_server_pipe () const -> server_pipe {
            auto path = path_.c_str ();

            // Temporarily set the umask to 0 so that any user can connect to our pipe.
            umask_raii umask (0);
            mode_t const mode = S_IFIFO | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
            if (::mkfifo (path, mode) >= 0) {
                int errcode = errno;
                if (errcode == EEXIST) {
                    // TODO: check the file owner and access permissions.
                    // If the object exists in the file system and it's a FIFO, then we assume that
                    // its ours (perhaps left behind after a previous crash?) and we use it.
                    struct stat buf {};
                    if (::stat (path, &buf) == 0 && S_ISFIFO (buf.st_mode)) {
                        errcode = 0;
                    }
                }

                // If we still have an error to deal with, throw.
                if (errcode != 0) {
                    std::ostringstream str;
                    str << "Could not create FIFO (" << path << ")";
                    raise (::pstore::errno_erc{errcode}, str.str ());
                }
            }

            needs_delete_ = true;

            // The server opens its well-known FIFO read-only (since it only reads from it) each
            // time the number of clients goes from 1 to 0, the server will read an end of file on
            // the FIFO. To prevent the server from having to handle this case, we use the trick of
            // just having the server open its well-known FIFO for read–write. Unfortunately,
            // POSIX.1 specifically states that opening a FIFO for read–write is undefined. Although
            // most UNIX systems allow this, we use two open() calls instead.

            int errcode = 0;
            unique_fd fdread = ::open (path, O_RDONLY | O_NONBLOCK); // NOLINT
            unique_fd fdwrite{};
            if (fdread.get () < 0) {
                errcode = errno;
            } else {
                fdwrite.reset (::open (path, O_WRONLY | O_NONBLOCK)); // NOLINT
                if (fdwrite.get () < 0) {
                    errcode = errno;
                }
            }

            if (errcode != 0) {
                std::ostringstream str;
                str << "Could not open FIFO (" << path << ")";
                raise (::pstore::errno_erc{errcode}, str.str ());
            }

            return {std::move (fdread), std::move (fdwrite)};
        }

        // open_impl
        // ~~~~~~~~~
        auto fifo_path::open_impl () const -> client_pipe {
            auto const & path = get ();

            // select() will return on EOF and for every EOF we handle there will be a new EOF which
            // results in read() calls spinning frantically. To avoid this we open the FIFO for both
            // reading and writing (O_RDWR). This ensures there is at least one writer on the FIFO
            // thus there won't be an EOF and as a result select() won't return unless someone
            // writes to the FIFO.

            client_pipe fdwrite = ::open (path.c_str (), O_WRONLY | O_NONBLOCK); // NOLINT
            if (fdwrite.get () < 0) {
                int const err = errno;

                // If O_NONBLOCK is specified an open for write-only returns –1 with errno set to
                // ENXIO if no process has the FIFO open for reading. In this event, or if the FIFO
                // wasn't found, return an invalid file descriptor. The caller may want to retry
                // once the service is ready to receive connections.
                if (err != ENOENT && err != ENXIO) {
                    std::ostringstream str;
                    str << "Could not open FIFO (" << path << ")";
                    raise (::pstore::errno_erc{err}, str.str ());
                }
            }
            return fdwrite;
        }

        // wait_until_impl
        // ~~~~~~~~~~~~~~~
        void fifo_path::wait_until_impl (std::chrono::milliseconds timeout) const {
            std::this_thread::sleep_for (timeout);
        }

        // get_default_path
        // ~~~~~~~~~~~~~~~~
        std::string fifo_path::get_default_path () {
            // TODO: consider using /run/user/<userid> on Linux?
            return std::string{"/var/tmp/"} + default_pipe_name;
        }

    } // namespace broker
} // namespace pstore

#endif //_WIN32
// eof: lib/pstore_broker_intf/fifo_path_posix.cpp
