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
//===- lib/brokerface/fifo_path_common.cpp --------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file fifo_path_common.cpp
/// \brief Portions of the implementation of the broker::fifo_path class that are common to all
///   target platforms.

#include "pstore/brokerface/fifo_path.hpp"

#include <sstream>
#include <thread>

#include "pstore/config/config.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/quoted.hpp"

namespace pstore {
    namespace brokerface {

        char const * const fifo_path::default_pipe_name = PSTORE_VENDOR_ID ".pstore_broker.fifo";

        // (ctor)
        // ~~~~~~
        fifo_path::fifo_path (gsl::czstring const pipe_path, duration_type const retry_timeout,
                              unsigned const max_retries, update_callback cb)
                : path_{pipe_path == nullptr ? get_default_path () : pipe_path}
                , retry_timeout_{retry_timeout}
                , max_retries_{max_retries}
                , update_cb_{std::move (cb)} {}

        fifo_path::fifo_path (gsl::czstring const pipe_path, update_callback cb)
                : fifo_path (pipe_path, duration_type{0}, 0, std::move (cb)) {}

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
                str << "Could not open client named pipe " << pstore::quoted (path_);
                raise (error_code::unable_to_open_named_pipe, str.str ());
            }
            return fd;
        }

    } // end namespace brokerface
} // end namespace pstore
