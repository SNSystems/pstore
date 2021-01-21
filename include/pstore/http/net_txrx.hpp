//*             _     _                   *
//*  _ __   ___| |_  | |___  ___ ____  __ *
//* | '_ \ / _ \ __| | __\ \/ / '__\ \/ / *
//* | | | |  __/ |_  | |_ >  <| |   >  <  *
//* |_| |_|\___|\__|  \__/_/\_\_|  /_/\_\ *
//*                                       *
//===- include/pstore/http/net_txrx.hpp -----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#ifndef PSTORE_HTTP_NET_TXRX_HPP
#define PSTORE_HTTP_NET_TXRX_HPP

#include "pstore/adt/error_or.hpp"
#include "pstore/os/descriptor.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace httpd {
        namespace net {

            // refiller
            // ~~~~~~~~
            /// Called when the buffered_reader<> needs more characters from the data stream.
            pstore::error_or<std::pair<socket_descriptor &, gsl::span<std::uint8_t>::iterator>>
            refiller (socket_descriptor & socket, gsl::span<std::uint8_t> const & s);

            error_or<socket_descriptor &> network_sender (socket_descriptor & socket,
                                                          gsl::span<std::uint8_t const> const & s);

        } // end namespace net
    }     // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTP_NET_TXRX_HPP
