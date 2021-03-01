//===- include/pstore/http/send.hpp -----------------------*- mode: C++ -*-===//
//*                     _  *
//*  ___  ___ _ __   __| | *
//* / __|/ _ \ '_ \ / _` | *
//* \__ \  __/ | | | (_| | *
//* |___/\___|_| |_|\__,_| *
//*                        *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_HTTP_SEND_HPP
#define PSTORE_HTTP_SEND_HPP

#include <cstdint>
#include <sstream>
#include <string>
#include <type_traits>

#include "pstore/adt/error_or.hpp"
#include "pstore/http/endian.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace http {

        static constexpr auto crlf = "\r\n";
        static constexpr auto server_name = "pstore-http";

        template <typename Sender, typename IO>
        error_or<IO> send (Sender sender, IO io, gsl::span<std::uint8_t const> const & s) {
            return sender (io, s);
        }
        template <typename Sender, typename IO>
        error_or<IO> send (Sender sender, IO io, std::string const & str) {
            auto * const data = reinterpret_cast<std::uint8_t const *> (str.data ());
            return send (sender, io, gsl::span<std::uint8_t const> (data, data + str.length ()));
        }
        template <typename Sender, typename IO>
        error_or<IO> send (Sender sender, IO io, std::ostringstream const & os) {
            return send (sender, io, os.str ());
        }

        template <typename Sender, typename IO, typename T,
                  typename = typename std::enable_if<std::is_integral<T>::value>::type>
        error_or<IO> send (Sender sender, IO io, T v) {
            T const nv = host_to_network (v);
            return send (sender, io, as_bytes (gsl::make_span (&nv, 1)));
        }

    } // end namespace http
} // end namespace pstore

#endif // PSTORE_HTTP_SEND_HPP
