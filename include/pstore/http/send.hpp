//*                     _  *
//*  ___  ___ _ __   __| | *
//* / __|/ _ \ '_ \ / _` | *
//* \__ \  __/ | | | (_| | *
//* |___/\___|_| |_|\__,_| *
//*                        *
//===- include/pstore/http/send.hpp ---------------------------------------===//
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
#ifndef PSTORE_HTTP_SEND_HPP
#define PSTORE_HTTP_SEND_HPP

#include <cstdint>
#include <sstream>
#include <string>
#include <type_traits>

#include "pstore/http/endian.hpp"
#include "pstore/support/error_or.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace httpd {

        static constexpr auto crlf = "\r\n";

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

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTP_SEND_HPP
