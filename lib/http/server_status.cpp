//*                                     _        _              *
//*  ___  ___ _ ____   _____ _ __   ___| |_ __ _| |_ _   _ ___  *
//* / __|/ _ \ '__\ \ / / _ \ '__| / __| __/ _` | __| | | / __| *
//* \__ \  __/ |   \ V /  __/ |    \__ \ || (_| | |_| |_| \__ \ *
//* |___/\___|_|    \_/ \___|_|    |___/\__\__,_|\__|\__,_|___/ *
//*                                                             *
//===- lib/http/server_status.cpp -----------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
