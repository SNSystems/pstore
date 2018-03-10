//*                _ _                                                         *
//* __      ___ __(_) |_ ___ _ __    ___ ___  _ __ ___  _ __ ___   ___  _ __   *
//* \ \ /\ / / '__| | __/ _ \ '__|  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
//*  \ V  V /| |  | | ||  __/ |    | (_| (_) | | | | | | | | | | | (_) | | | | *
//*   \_/\_/ |_|  |_|\__\___|_|     \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
//*                                                                            *
//===- lib/broker_intf/writer_common.cpp ----------------------------------===//
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
/// \file writer_common.cpp
/// \brief Implements the parts of the class which enables a client to send messages to the broker
/// which are common to all target operating systems.

#include "pstore/broker_intf/writer.hpp"
#include <thread>
#include <utility>
#include "pstore/support/error.hpp"

namespace pstore {
    namespace broker {

        // (ctor)
        // ~~~~~~
        writer::writer (fifo_path::client_pipe && pipe, duration_type retry_timeout,
                        unsigned max_retries, update_callback cb)
                : fd_{std::move (pipe)}
                , retry_timeout_{retry_timeout}
                , max_retries_{max_retries}
                , update_cb_{std::move (cb)} {}

        writer::writer (fifo_path::client_pipe && pipe, update_callback cb)
                : writer (std::move (pipe), duration_type{0}, 0U, cb) {}

        writer::writer (fifo_path const & fifo, duration_type retry_timeout, unsigned max_retries,
                        update_callback cb)
                : fd_{fifo.open_client_pipe ()}
                , retry_timeout_{retry_timeout}
                , max_retries_{max_retries}
                , update_cb_{std::move (cb)} {}

        writer::writer (fifo_path const & fifo, update_callback cb)
                : writer (fifo, duration_type{0}, 0U, cb) {}

        // default_callback [static]
        // ~~~~~~~~~~~~~~~~
        void writer::default_callback () {}

        // write
        // ~~~~~
        void writer::write (message_type const & msg, bool error_on_timeout) {
            auto retries = 0U;
            do {
                update_cb_ ();
                if (this->write_impl (msg)) {
                    break;
                }
                std::this_thread::sleep_for (retry_timeout_);
            } while (retries++ < max_retries_);

            if (error_on_timeout && retries > max_retries_) {
                raise (::pstore::error_code::pipe_write_timeout);
            }
        }

    } // namespace broker
} // namespace pstore
// eof: lib/broker_intf/writer_common.cpp
