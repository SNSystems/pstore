//*                                     _        _              *
//*  ___  ___ _ ____   _____ _ __   ___| |_ __ _| |_ _   _ ___  *
//* / __|/ _ \ '__\ \ / / _ \ '__| / __| __/ _` | __| | | / __| *
//* \__ \  __/ |   \ V /  __/ |    \__ \ || (_| | |_| |_| \__ \ *
//* |___/\___|_|    \_/ \___|_|    |___/\__\__,_|\__|\__,_|___/ *
//*                                                             *
//===- include/pstore/http/server_status.hpp ------------------------------===//
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
#ifndef PSTORE_HTTP_SERVER_STATUS_HPP
#define PSTORE_HTTP_SERVER_STATUS_HPP

#include <atomic>

#include "pstore/broker_intf/descriptor.hpp" // for in_port_t

namespace pstore {
    namespace httpd {

        class server_status {
        public:
            explicit server_status (in_port_t port);

            server_status (server_status const & rhs) = default;
            server_status (server_status && rhs) = default;
            server_status & operator= (server_status const & rhs) = delete;
            server_status & operator= (server_status && rhs) = delete;

            void shutdown () noexcept;

            enum class http_state { initializing, listening, closing };
            std::atomic<http_state> state;
            in_port_t const port;
        };

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTP_SERVER_STATUS_HPP
