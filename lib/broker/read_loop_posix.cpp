//*                     _   _                    *
//*  _ __ ___  __ _  __| | | | ___   ___  _ __   *
//* | '__/ _ \/ _` |/ _` | | |/ _ \ / _ \| '_ \  *
//* | | |  __/ (_| | (_| | | | (_) | (_) | |_) | *
//* |_|  \___|\__,_|\__,_| |_|\___/ \___/| .__/  *
//*                                      |_|     *
//===- lib/broker/read_loop_posix.cpp -------------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
/// \file read_loop_posix.cpp

#include "pstore/broker/read_loop.hpp"

#ifndef _WIN32

// Platform includes
#    include <sys/select.h>
#    include <sys/types.h>
#    include <sys/uio.h>

// pstore includes
#    include "pstore/broker/command.hpp"
#    include "pstore/broker/globals.hpp"
#    include "pstore/broker/message_pool.hpp"
#    include "pstore/broker/quit.hpp"
#    include "pstore/broker/recorder.hpp"
#    include "pstore/os/logging.hpp"

namespace {

    using priority = pstore::logger::priority;

    // block for input
    // ~~~~~~~~~~~~~~~
    /// Watch fd to be notified when it has input.
    void block_for_input (int const fd) {
        timeval timeout{pstore::broker::details::timeout_seconds, 0};
        fd_set rfds;
        FD_ZERO (&rfds);    // NOLINT
        FD_SET (fd, &rfds); // NOLINT

        fd_set efds;
        FD_ZERO (&efds);    // NOLINT
        FD_SET (fd, &efds); // NOLINT

        int const retval = select (fd + 1, &rfds, nullptr, &efds, &timeout);
        if (retval == -1) {
            raise (pstore::errno_erc{errno}, "select");
        } else if (retval == 0) {
            log (priority::notice, "no data within timeout");
        }
    }

} // end anonymous namespace


namespace pstore {
    namespace broker {

        // read loop
        // ~~~~~~~~~
        void read_loop (brokerface::fifo_path & fifo, std::shared_ptr<recorder> & record_file,
                        std::shared_ptr<command_processor> const cp) {
            try {
                log (priority::notice, "listening to FIFO ", logger::quoted{fifo.get ().c_str ()});
                auto const fd = fifo.open_server_pipe ();

                brokerface::message_ptr readbuf = pool.get_from_pool ();

                for (;;) {
                    while (ssize_t const bytes_read =
                               ::read (fd.native_handle (), readbuf.get (), sizeof (*readbuf))) {
                        if (bytes_read < 0) {
                            int const err = errno;
                            if (err == EAGAIN || err == EWOULDBLOCK) {
                                // Data ran out so wait for more to arrive.
                                break;
                            }
                            raise (errno_erc{err}, "read");
                        }

                        if (done) {
                            log (priority::notice, "exiting read loop");
                            return;
                        }

                        if (bytes_read != brokerface::message_size) {
                            log (priority::error, "Partial message received. Length ", bytes_read);
                        } else {
                            // Push the command buffer on to the queue for processing and pull
                            // an new read buffer from the pool.
                            PSTORE_ASSERT (static_cast<std::size_t> (bytes_read) <=
                                           sizeof (*readbuf));
                            cp->push_command (std::move (readbuf), record_file.get ());

                            readbuf = pool.get_from_pool ();
                        }
                    }

                    // This function will return when data is available on the pipe. Be aware that
                    // another thread may also wake in response to its presence. Bear in mind that
                    // that other thread may read the data before this one attempts to do so
                    // (resulting in EWOULDBLOCK).
                    block_for_input (fd.native_handle ());
                }
            } catch (std::exception const & ex) {
                log (priority::error, "error: ", ex.what ());
                exit_code = EXIT_FAILURE;
                notify_quit_thread ();
            } catch (...) {
                log (priority::error, "unknown error");
                exit_code = EXIT_FAILURE;
                notify_quit_thread ();
            }
            log (priority::notice, "exiting read loop");
        }

    } // end namespace broker
} // end namespace pstore

#endif // _WIN32
