//*                                     _        _              *
//*  ___  ___ _ ____   _____ _ __   ___| |_ __ _| |_ _   _ ___  *
//* / __|/ _ \ '__\ \ / / _ \ '__| / __| __/ _` | __| | | / __| *
//* \__ \  __/ |   \ V /  __/ |    \__ \ || (_| | |_| |_| \__ \ *
//* |___/\___|_|    \_/ \___|_|    |___/\__\__,_|\__|\__,_|___/ *
//*                                                             *
//===- lib/http/server_status.cpp -----------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
/// \file server_status.cpp
/// \brief Implements the server_status class representing the state of the HTTP server.
#include "pstore/http/server_status.hpp"

#ifdef _WIN32
#    include <ws2tcpip.h>
#endif

#include "pstore/http/endian.hpp"
#include "pstore/support/error.hpp"

namespace pstore {
    namespace httpd {

        //*                               _        _            *
        //*  ___ ___ _ ___ _____ _ _   __| |_ __ _| |_ _  _ ___ *
        //* (_-</ -_) '_\ V / -_) '_| (_-<  _/ _` |  _| || (_-< *
        //* /__/\___|_|  \_/\___|_|   /__/\__\__,_|\__|\_,_/__/ *
        //*                                                     *
        // set real port number
        // ~~~~~~~~~~~~~~~~~~~~
        in_port_t server_status::set_real_port_number (socket_descriptor const & descriptor) {
            std::lock_guard<decltype (mut_)> lock{mut_};
            if (port_ == 0) {
                sockaddr_in address{};
                socklen_t address_len = sizeof (address);
                if (getsockname (descriptor.native_handle (),
                                 reinterpret_cast<sockaddr *> (&address), &address_len) != 0) {
                    raise (errno_erc{errno}, "getsockname");
                }

                auto const port = network_to_host (address.sin_port);
                PSTORE_ASSERT (port != 0);
                port_ = port;
            }
            return port_;
        }

        // port
        // ~~~~
        in_port_t server_status::port () const {
            std::lock_guard<decltype (mut_)> lock{mut_};
            return port_;
        }

    } // end namespace httpd
} // end namespace pstore
