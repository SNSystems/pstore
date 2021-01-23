//*             _     _                   *
//*  _ __   ___| |_  | |___  ___ ____  __ *
//* | '_ \ / _ \ __| | __\ \/ / '__\ \/ / *
//* | | | |  __/ |_  | |_ >  <| |   >  <  *
//* |_| |_|\___|\__|  \__/_/\_\_|  /_/\_\ *
//*                                       *
//===- lib/http/net_txrx.cpp ----------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
    constexpr bool is_recv_error (ssize_t const nread) noexcept {
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
    namespace http {
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
                PSTORE_ASSERT (sizeof (size_type) < sizeof (size) ||
                               static_cast<size_type> (size) <
                                   std::numeric_limits<size_type>::max ());

                errno = 0;
                ssize_t const nread =
                    ::recv (socket.native_handle (), reinterpret_cast<char *> (s.data ()),
                            static_cast<size_type> (size), 0 /*flags*/);
                PSTORE_ASSERT (is_recv_error (nread) || (nread >= 0 && nread <= size));
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
                PSTORE_ASSERT (sizeof (size_type) < sizeof (size) ||
                               static_cast<size_type> (size) <
                                   std::numeric_limits<size_type>::max ());

                if (::send (socket.native_handle (), reinterpret_cast<data_type> (s.data ()),
                            static_cast<size_type> (size), 0 /*flags*/) < 0) {
                    return result_type{get_last_error ()};
                }
                return result_type{socket};
            }

        } // end namespace net
    }     // end namespace http
} // end namespace pstore
