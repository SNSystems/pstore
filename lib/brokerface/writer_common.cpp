//===- lib/brokerface/writer_common.cpp -----------------------------------===//
//*                _ _                                                         *
//* __      ___ __(_) |_ ___ _ __    ___ ___  _ __ ___  _ __ ___   ___  _ __   *
//* \ \ /\ / / '__| | __/ _ \ '__|  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
//*  \ V  V /| |  | | ||  __/ |    | (_| (_) | | | | | | | | | | | (_) | | | | *
//*   \_/\_/ |_|  |_|\__\___|_|     \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
//*                                                                            *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file writer_common.cpp
/// \brief Implements the parts of the class which enables a client to send messages to the broker
/// which are common to all target operating systems.

#include "pstore/brokerface/writer.hpp"

#include <thread>
#include <utility>

#include "pstore/support/error.hpp"

namespace pstore {
    namespace brokerface {

        // (ctor)
        // ~~~~~~
        writer::writer (fifo_path::client_pipe && pipe, duration_type const retry_timeout,
                        unsigned const max_retries, update_callback cb)
                : fd_{std::move (pipe)}
                , retry_timeout_{retry_timeout}
                , max_retries_{max_retries}
                , update_cb_{std::move (cb)} {}

        writer::writer (fifo_path::client_pipe && pipe, update_callback cb)
                : writer (std::move (pipe), duration_type{0}, 0U, std::move (cb)) {}

        writer::writer (fifo_path const & fifo, duration_type const retry_timeout,
                        unsigned const max_retries, update_callback cb)
                : fd_{fifo.open_client_pipe ()}
                , retry_timeout_{retry_timeout}
                , max_retries_{max_retries}
                , update_cb_{std::move (cb)} {}

        writer::writer (fifo_path const & fifo, update_callback cb)
                : writer (fifo, duration_type{0}, 0U, std::move (cb)) {}

        // default_callback [static]
        // ~~~~~~~~~~~~~~~~
        void writer::default_callback () {}

        // write
        // ~~~~~
        void writer::write (message_type const & msg, bool const error_on_timeout) {
            auto tries = 0U;
            bool const infinite_tries = max_retries_ == infinite_retries;
            for (; infinite_tries || tries <= max_retries_; ++tries) {
                update_cb_ ();
                if (this->write_impl (msg)) {
                    break;
                }
                std::this_thread::sleep_for (retry_timeout_);
            }

            if (error_on_timeout && !infinite_tries && tries > max_retries_) {
                raise (::pstore::error_code::pipe_write_timeout);
            }
        }

    } // end namespace brokerface
} // end namespace pstore
