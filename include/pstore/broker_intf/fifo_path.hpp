//*   __ _  __                     _   _      *
//*  / _(_)/ _| ___    _ __   __ _| |_| |__   *
//* | |_| | |_ / _ \  | '_ \ / _` | __| '_ \  *
//* |  _| |  _| (_) | | |_) | (_| | |_| | | | *
//* |_| |_|_|  \___/  | .__/ \__,_|\__|_| |_| *
//*                   |_|                     *
//===- include/pstore/broker_intf/fifo_path.hpp ---------------------------===//
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
/// \file fifo_path.hpp

#ifndef PSTORE_BROKER_INTF_FIFO_PATH_HPP
#define PSTORE_BROKER_INTF_FIFO_PATH_HPP

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <tuple>

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace broker {

        class fifo_path {
        public:
            using duration_type = std::chrono::milliseconds;
#ifdef _WIN32
            using client_pipe = unique_handle;
            using server_pipe = unique_handle;
#else
            using client_pipe = unique_fd;

            class server_pipe {
            public:
                using value_type = unique_fd::value_type;

                server_pipe (unique_fd && read, unique_fd && write)
                        : fd_{std::move (read), std::move (write)} {}
                ~server_pipe () = default;
                server_pipe (server_pipe &&) noexcept = default;
                server_pipe & operator= (server_pipe &&) noexcept = default;

                // No copying or assignment.
                server_pipe (server_pipe const &) = delete;
                server_pipe & operator= (server_pipe const &) = delete;

                value_type get () const noexcept { return read_pipe ().get (); }
                bool valid () const noexcept { return read_pipe ().valid (); }

            private:
                std::tuple<unique_fd, unique_fd> fd_;
                unique_fd const & read_pipe () const { return std::get<0> (fd_); }
            };
#endif
            enum class operation { open, wait };
            using update_callback = std::function<void(operation)>;
            static void default_update_cb (operation) {}


            /// \param pipe_path  The name of the pipe to use for the FIFO. If nullptr, the default
            /// path (as defined at configure time) is used.
            fifo_path (gsl::czstring pipe_path, update_callback cb = default_update_cb);

            /// \param pipe_path  The name of the pipe to use for the FIFO. If nullptr, the default
            /// path (as defined at configure time) is used.
            fifo_path (gsl::czstring pipe_path, duration_type retry_timeout, unsigned max_retries,
                       update_callback cb = default_update_cb);
            ~fifo_path ();

            fifo_path (fifo_path && rhs) = default;
            fifo_path (fifo_path const & rhs) = delete;
            fifo_path & operator= (fifo_path && rhs) = default;
            fifo_path & operator= (fifo_path const & rhs) = delete;

#ifndef _WIN32
            /// Create the pipe and open a read descriptor. (Used by the pipe server.)
            server_pipe open_server_pipe () const;
#endif
            /// Open the pipe for writing. (Used by clients to write commands to the pipe.)
            client_pipe open_client_pipe () const;

            std::string const & get () const { return path_; }

        private:
            static char const * const default_pipe_name;

            static std::string get_default_path ();
            client_pipe open_impl () const;
            void wait_until_impl (std::chrono::milliseconds timeout) const;

            mutable std::atomic<bool> needs_delete_{false};
            std::string const path_;

            duration_type const retry_timeout_;
            unsigned const max_retries_;
            update_callback const update_cb_;
        };

    } // namespace broker
} // namespace pstore

#endif // PSTORE_BROKER_INTF_FIFO_PATH_HPP
// eof: include/pstore/broker_intf/fifo_path.hpp