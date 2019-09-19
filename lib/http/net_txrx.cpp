//*             _     _                   *
//*  _ __   ___| |_  | |___  ___ ____  __ *
//* | '_ \ / _ \ __| | __\ \/ / '__\ \/ / *
//* | | | |  __/ |_  | |_ >  <| |   >  <  *
//* |_| |_|\___|\__|  \__/_/\_\_|  /_/\_\ *
//*                                       *
//===- lib/http/net_txrx.cpp ----------------------------------------------===//
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
#include "pstore/http/net_txrx.hpp"

#ifdef _WIN32
#    include <winsock2.h>
#else
#    include <sys/socket.h>
#endif // _WIN32

#include "pstore/http/error.hpp"
#include "pstore/support/portab.hpp"

namespace {

    // is_recv_error
    // ~~~~~~~~~~~~~
    constexpr bool is_recv_error (ssize_t nread) noexcept {
#ifdef _WIN32
        return nread == SOCKET_ERROR;
#else
        return nread < 0;
#endif // !_WIN32
    }

#ifdef _WIN32
    using size_type = int;
    using data_type = char const *;
#else
    using size_type = std::size_t;
    using data_type = std::uint8_t const *;
#endif // _!WIN32

} // end anonymous namespace


namespace pstore {
    namespace httpd {
        namespace net {

            // refiller
            // ~~~~~~~~
            /// Called when the buffered_reader<> needs more characters from the data stream.
            error_or<std::pair<socket_descriptor &, gsl::span<std::uint8_t>::iterator>>
            refiller (socket_descriptor & socket, gsl::span<std::uint8_t> const & s) {
                using result_type =
                    error_or<std::pair<socket_descriptor &, gsl::span<std::uint8_t>::iterator>>;

                auto size = s.size ();
                size = std::max (size, decltype (size){0});
                assert (sizeof (size_type) < sizeof (size) ||
                        static_cast<size_type> (size) < std::numeric_limits<size_type>::max ());

                errno = 0;
                ssize_t const nread =
                    ::recv (socket.native_handle (), reinterpret_cast<char *> (s.data ()),
                            static_cast<size_type> (size), 0 /*flags*/);
                assert (is_recv_error (nread) || (nread >= 0 && nread <= size));
                if (is_recv_error (nread)) {
                    return result_type{get_last_error ()};
                }
                return result_type{in_place, socket, s.begin () + nread};
            }

            error_or<socket_descriptor &> network_sender (socket_descriptor & socket,
                                                          gsl::span<std::uint8_t const> const & s) {
                using result_type = error_or<socket_descriptor &>;

                auto size = s.size ();
                size = std::max (size, decltype (size){0});
                assert (sizeof (size_type) < sizeof (size) ||
                        static_cast<size_type> (size) < std::numeric_limits<size_type>::max ());

                if (::send (socket.native_handle (), reinterpret_cast<data_type> (s.data ()),
                            static_cast<size_type> (s.size ()), 0 /*flags*/) < 0) {
                    return result_type{get_last_error ()};
                }
                return result_type{socket};
            }

        } // end namespace net
    }     // end namespace httpd
} // end namespace pstore
