//*             _     _                   *
//*  _ __   ___| |_  | |___  ___ ____  __ *
//* | '_ \ / _ \ __| | __\ \/ / '__\ \/ / *
//* | | | |  __/ |_  | |_ >  <| |   >  <  *
//* |_| |_|\___|\__|  \__/_/\_\_|  /_/\_\ *
//*                                       *
//===- include/pstore/http/net_txrx.hpp -----------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
