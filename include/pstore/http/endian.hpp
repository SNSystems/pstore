//*                 _ _              *
//*   ___ _ __   __| (_) __ _ _ __   *
//*  / _ \ '_ \ / _` | |/ _` | '_ \  *
//* |  __/ | | | (_| | | (_| | | | | *
//*  \___|_| |_|\__,_|_|\__,_|_| |_| *
//*                                  *
//===- include/pstore/http/endian.hpp -------------------------------------===//
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
#ifndef PSTORE_HTTP_ENDIAN_HPP
#define PSTORE_HTTP_ENDIAN_HPP

#include "pstore/config/config.hpp"

#ifdef _WIN32
#    include <winsock2.h>
#else
#    include <arpa/inet.h>

#    if PSTORE_HAVE_BYTESWAP_H
#        include <byteswap.h>
#    endif
#    if PSTORE_HAVE_SYS_ENDIAN_H
#        include <sys/endian.h>
#    endif

#endif

namespace pstore {
    namespace httpd {

        template <typename T>
        T network_to_host (T) noexcept;
        template <>
        inline std::uint16_t network_to_host<std::uint16_t> (std::uint16_t x) noexcept {
            return ntohs (x); // NOLINT
        }
        template <>
        inline std::uint32_t network_to_host<std::uint32_t> (std::uint32_t x) noexcept {
            return ntohl (x); // NOLINT
        }
        template <>
        inline std::uint64_t network_to_host<std::uint64_t> (std::uint64_t x) noexcept {
#if PSTORE_IS_BIG_ENDIAN
            return x;
#else
#    if PSTORE_HAVE_BYTESWAP_H
            return bswap_64 (x);
#    elif PSTORE_HAVE_SYS_ENDIAN_H
            return bswap64 (x);
#    else
            return ntohll (x); // NOLINT
#    endif // PSTORE_HAVE_BYTESWAP_H
#endif     // PSTORE_IS_BIG_ENDIAN
        }



        template <typename T>
        T host_to_network (T) noexcept;
        template <>
        inline std::uint16_t host_to_network<std::uint16_t> (std::uint16_t x) noexcept {
            return htons (x); // NOLINT
        }
        template <>
        inline std::uint32_t host_to_network<std::uint32_t> (std::uint32_t x) noexcept {
            return htonl (x); // NOLINT
        }
        template <>
        inline std::uint64_t host_to_network<std::uint64_t> (std::uint64_t x) noexcept {
#if PSTORE_IS_BIG_ENDIAN
            return x;
#else
#    if PSTORE_HAVE_BYTESWAP_H
            return bswap_64 (x);
#    elif PSTORE_HAVE_SYS_ENDIAN_H
            return bswap64 (x);
#    else
            return ntohll (x); // NOLINT
#    endif // PSTORE_HAVE_BYTESWAP_H
#endif     // PSTORE_IS_BIG_ENDIAN
        }


    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTP_ENDIAN_HPP
