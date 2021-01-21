//*                _ _             *
//* __      ___ __(_) |_ ___ _ __  *
//* \ \ /\ / / '__| | __/ _ \ '__| *
//*  \ V  V /| |  | | ||  __/ |    *
//*   \_/\_/ |_|  |_|\__\___|_|    *
//*                                *
//===- include/pstore/brokerface/writer.hpp -------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file writer.hpp
/// \brief Provides a simple interface to enable a client to send messages to the pstore broker.

#ifndef PSTORE_BROKERFACE_WRITER_HPP
#define PSTORE_BROKERFACE_WRITER_HPP

#include <cstdlib>

// pstore broker-interface
#include "pstore/brokerface/fifo_path.hpp"

namespace pstore {
    namespace brokerface {

        class message_type;

        class writer {
        public:
            using duration_type = std::chrono::milliseconds;
            static constexpr unsigned infinite_retries = std::numeric_limits<unsigned>::max ();

            using update_callback = std::function<void ()>;
            /// The default function called by write() to as the write() function retries the
            /// the write operation. A simple no-op.
            static void default_callback ();

            writer (fifo_path::client_pipe && pipe, duration_type retry_timeout,
                    unsigned max_retries, update_callback cb = default_callback);
            writer (fifo_path::client_pipe && pipe, update_callback cb = default_callback);

            writer (fifo_path const & fifo, duration_type retry_timeout, unsigned max_retries,
                    update_callback cb = default_callback);
            writer (fifo_path const & fifo, update_callback cb = default_callback);
            writer (writer const &) = delete;
            writer (writer && rhs) noexcept = default;

            virtual ~writer () = default;

            writer & operator= (writer const &) = delete;
            writer & operator= (writer && rhs) noexcept = default;

            void write (message_type const & msg, bool error_on_timeout);

        private:
            // (virtual for unit testing)
            virtual bool write_impl (message_type const & msg);

            /// The pipe to which the write() function will write data.
            fifo_path::client_pipe fd_;
            /// The time delay between retries.
            duration_type retry_timeout_;
            /// The number of retries that will be attempted before write() will give up.
            unsigned max_retries_;
            /// A function which is called by write() before each retry.
            update_callback update_cb_;
        };

    } // namespace brokerface
} // namespace pstore

#endif // PSTORE_BROKERFACE_WRITER_HPP
