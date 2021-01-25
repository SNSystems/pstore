//*   __ _  __                     _   _      *
//*  / _(_)/ _| ___    _ __   __ _| |_| |__   *
//* | |_| | |_ / _ \  | '_ \ / _` | __| '_ \  *
//* |  _| |  _| (_) | | |_) | (_| | |_| | | | *
//* |_| |_|_|  \___/  | .__/ \__,_|\__|_| |_| *
//*                   |_|                     *
//===- include/pstore/brokerface/fifo_path.hpp ----------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file fifo_path.hpp

#ifndef PSTORE_BROKERFACE_FIFO_PATH_HPP
#define PSTORE_BROKERFACE_FIFO_PATH_HPP

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <tuple>

#include "pstore/os/descriptor.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/portab.hpp"

namespace pstore {
    namespace brokerface {

        class fifo_path {
        public:
            using duration_type = std::chrono::milliseconds;
            using client_pipe = pipe_descriptor;
#ifdef _WIN32
            using server_pipe = pipe_descriptor;
#else
            class server_pipe {
            public:
                using value_type = pipe_descriptor::value_type;

                server_pipe (pipe_descriptor && read, pipe_descriptor && write)
                        : fd_{std::move (read), std::move (write)} {}
                // No copying or assignment.
                server_pipe (server_pipe const &) = delete;
                server_pipe (server_pipe &&) noexcept = default;

                ~server_pipe () = default;

                // No copying or assignment.
                server_pipe & operator= (server_pipe const &) = delete;
                server_pipe & operator= (server_pipe &&) noexcept = default;

                value_type native_handle () const noexcept { return read_pipe ().native_handle (); }
                bool valid () const noexcept { return read_pipe ().valid (); }

            private:
                std::tuple<pipe_descriptor, pipe_descriptor> fd_;
                pipe_descriptor const & read_pipe () const { return std::get<0> (fd_); }
            };
#endif
            enum class operation { open, wait };
            using update_callback = std::function<void (operation)>;
            static constexpr void default_update_cb (operation const) noexcept {}
            static constexpr unsigned infinite_retries = std::numeric_limits<unsigned>::max ();

            /// \param pipe_path  The name of the pipe to use for the FIFO. If nullptr, the default
            /// path (as defined at configure time) is used.
            explicit fifo_path (gsl::czstring pipe_path, update_callback cb = default_update_cb);

            /// \param pipe_path  The name of the pipe to use for the FIFO. If nullptr, the default
            /// path (as defined at configure time) is used.
            fifo_path (gsl::czstring pipe_path, duration_type retry_timeout, unsigned max_retries,
                       update_callback cb = default_update_cb);
            // No copying or assignment.
            fifo_path (fifo_path const & rhs) = delete;
            fifo_path (fifo_path && rhs) noexcept = delete;
            ~fifo_path ();

            // No copying or assignment.
            fifo_path & operator= (fifo_path const & rhs) = delete;
            fifo_path & operator= (fifo_path && rhs) noexcept = delete;

#ifndef _WIN32
            /// Create the pipe and open a read descriptor. (Used by the pipe server.)
            server_pipe open_server_pipe ();
#endif
            /// Open the pipe for writing. (Used by clients to write commands to the pipe.)
            client_pipe open_client_pipe () const;

            std::string const & get () const { return path_; }

        private:
            static char const * const default_pipe_name;

            static std::string get_default_path ();
            client_pipe open_impl () const;
            void wait_until_impl (std::chrono::milliseconds timeout) const;

#ifndef _WIN32
            /// A mutex to prevent more than one thread trying to create/open the server pipe at the
            /// same time.
            std::mutex open_server_pipe_mut_;
#endif
            std::atomic<bool> needs_delete_{false};
            std::string const path_;

            duration_type const retry_timeout_;
            unsigned const max_retries_;
            update_callback const update_cb_;
        };

    } // end namespace brokerface
} // end namespace pstore

#endif // PSTORE_BROKERFACE_FIFO_PATH_HPP
