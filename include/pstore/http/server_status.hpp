//*                                     _        _              *
//*  ___  ___ _ ____   _____ _ __   ___| |_ __ _| |_ _   _ ___  *
//* / __|/ _ \ '__\ \ / / _ \ '__| / __| __/ _` | __| | | / __| *
//* \__ \  __/ |   \ V /  __/ |    \__ \ || (_| | |_| |_| \__ \ *
//* |___/\___|_|    \_/ \___|_|    |___/\__\__,_|\__|\__,_|___/ *
//*                                                             *
//===- include/pstore/http/server_status.hpp ------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file server_status.hpp
/// \brief Defines the server_status class which represents the state of the HTTP server.

#ifndef PSTORE_HTTP_SERVER_STATUS_HPP
#define PSTORE_HTTP_SERVER_STATUS_HPP

#include <atomic>
#include <mutex>

#include "pstore/os/descriptor.hpp"
#include "pstore/support/assert.hpp"

namespace pstore {
    namespace http {

        //*                               _        _            *
        //*  ___ ___ _ ___ _____ _ _   __| |_ __ _| |_ _  _ ___ *
        //* (_-</ -_) '_\ V / -_) '_| (_-<  _/ _` |  _| || (_-< *
        //* /__/\___|_|  \_/\___|_|   /__/\__\__,_|\__|\_,_/__/ *
        //*                                                     *
        class server_status {
        public:
            explicit server_status (in_port_t const port) noexcept
                    : port_{port} {}
            server_status (server_status const & rhs) = delete;
            server_status (server_status && rhs) noexcept
                    : state_{rhs.state_.load ()}
                    , port_{rhs.port_} {}

            ~server_status () noexcept = default;

            server_status & operator= (server_status const & rhs) = delete;
            server_status & operator= (server_status && rhs) = delete;

            enum class http_state { initializing, listening, closing };

            /// Sets the server's state to "closing" and returns the old state.
            http_state set_state_to_shutdown () noexcept {
                return state_.exchange (http_state::closing);
            }

            /// Sets the current server state to 'listening' and return true if it is currently \p
            /// expected. Otherwise, false is returned.
            ///
            /// \p expected  If the server is in the expected state, return true otherwise false.
            /// \result True if the server was in the expected state, otherwise false.
            bool listening (http_state expected) noexcept {
                return state_.compare_exchange_strong (expected,
                                                       server_status::http_state::listening);
            }

            /// If the original port number (as passed to the ctor) was 0, the system will allocate
            /// a free ephemeral port number. Call this function to record the actual allocated port
            /// number.
            in_port_t set_real_port_number (socket_descriptor const & descriptor);

            /// Returns the port number in use by the server. In most cases, call
            /// set_real_port_number() beforehand to ensure that the actual port number is returned.
            in_port_t port () const;

        private:
            std::atomic<http_state> state_{http_state::initializing};

            mutable std::mutex mut_;
            in_port_t port_;
        };

    } // end namespace http
} // end namespace pstore

#endif // PSTORE_HTTP_SERVER_STATUS_HPP
