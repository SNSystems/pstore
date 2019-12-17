//*   __ _  __                     _   _      *
//*  / _(_)/ _| ___    _ __   __ _| |_| |__   *
//* | |_| | |_ / _ \  | '_ \ / _` | __| '_ \  *
//* |  _| |  _| (_) | | |_) | (_| | |_| | | | *
//* |_| |_|_|  \___/  | .__/ \__,_|\__|_| |_| *
//*                   |_|                     *
//*                                             *
//*   ___ ___  _ __ ___  _ __ ___   ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
//* | (_| (_) | | | | | | | | | | | (_) | | | | *
//*  \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
//*                                             *
//===- lib/broker_intf/fifo_path_common.cpp -------------------------------===//
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
/// \file fifo_path_common.cpp
/// \brief Portions of the implementation of the broker::fifo_path class that are common to all
///   target platforms.

#include "pstore/broker_intf/fifo_path.hpp"

#include <sstream>
#include <thread>

#include "pstore/config/config.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/quoted_string.hpp"

namespace pstore {
    namespace broker {

        char const * const fifo_path::default_pipe_name = PSTORE_VENDOR_ID ".pstore_broker.fifo";

        // (ctor)
        // ~~~~~~
        fifo_path::fifo_path (gsl::czstring const pipe_path, duration_type const retry_timeout,
                              unsigned const max_retries, update_callback const cb)
                : path_{pipe_path == nullptr ? get_default_path () : pipe_path}
                , retry_timeout_{retry_timeout}
                , max_retries_{max_retries}
                , update_cb_{std::move (cb)} {}

        fifo_path::fifo_path (gsl::czstring const pipe_path, update_callback const cb)
                : fifo_path (pipe_path, duration_type{0}, 0, cb) {}

        // open
        // ~~~~
        auto fifo_path::open_client_pipe () const -> client_pipe {
            client_pipe fd{};
            auto tries = 0U;
            for (; max_retries_ == infinite_retries || tries <= max_retries_; ++tries) {
                update_cb_ (operation::open);
                fd = this->open_impl ();
                if (fd.valid ()) {
                    break;
                }
                update_cb_ (operation::wait);
                this->wait_until_impl (retry_timeout_);
            }

            if (!fd.valid ()) {
                std::ostringstream str;
                str << "Could not open client named pipe " << quoted (path_);
                raise (pstore::error_code::unable_to_open_named_pipe, str.str ());
            }
            return fd;
        }

    } // namespace broker
} // namespace pstore
