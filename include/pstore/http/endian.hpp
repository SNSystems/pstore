//*                 _ _              *
//*   ___ _ __   __| (_) __ _ _ __   *
//*  / _ \ '_ \ / _` | |/ _` | '_ \  *
//* |  __/ | | | (_| | | (_| | | | | *
//*  \___|_| |_|\__,_|_|\__,_|_| |_| *
//*                                  *
//===- include/pstore/http/endian.hpp -------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file endian.hpp
/// \brief Network/host byte swapping.

#ifndef PSTORE_HTTP_ENDIAN_HPP
#define PSTORE_HTTP_ENDIAN_HPP

#include "pstore/config/config.hpp"

#ifdef _WIN32
#    include <winsock2.h>
#else
#    include <arpa/inet.h>

#    ifdef PSTORE_HAVE_BYTESWAP_H
#        include <byteswap.h>
#    endif
#    ifdef PSTORE_HAVE_SYS_ENDIAN_H
#        include <sys/endian.h>
#    endif

#endif

namespace pstore {
    namespace http {

        template <typename T>
        T network_to_host (T x) noexcept;
        template <>
        inline std::uint16_t network_to_host<std::uint16_t> (std::uint16_t const x) noexcept {
            return ntohs (x); // NOLINT
        }
        template <>
        inline std::uint32_t network_to_host<std::uint32_t> (std::uint32_t const x) noexcept {
            return ntohl (x); // NOLINT
        }
        template <>
        inline std::uint64_t network_to_host<std::uint64_t> (std::uint64_t const x) noexcept {
#ifdef PSTORE_IS_BIG_ENDIAN
            return x;
#else
#    ifdef PSTORE_HAVE_BYTESWAP_H
            return bswap_64 (x);
#    elif defined(PSTORE_HAVE_SYS_ENDIAN_H)
            return bswap64 (x);
#    else
            return ntohll (x); // NOLINT
#    endif // PSTORE_HAVE_BYTESWAP_H
#endif     // PSTORE_IS_BIG_ENDIAN
        }


        template <typename T>
        T host_to_network (T x) noexcept;
        template <>
        inline std::uint16_t host_to_network<std::uint16_t> (std::uint16_t const x) noexcept {
            return htons (x); // NOLINT
        }
        template <>
        inline std::uint32_t host_to_network<std::uint32_t> (std::uint32_t const x) noexcept {
            return htonl (x); // NOLINT
        }
        template <>
        inline std::uint64_t host_to_network<std::uint64_t> (std::uint64_t const x) noexcept {
#ifdef PSTORE_IS_BIG_ENDIAN
            return x;
#else
#    ifdef PSTORE_HAVE_BYTESWAP_H
            return bswap_64 (x);
#    elif defined(PSTORE_HAVE_SYS_ENDIAN_H)
            return bswap64 (x);
#    else
            return ntohll (x); // NOLINT
#    endif // PSTORE_HAVE_BYTESWAP_H
#endif     // PSTORE_IS_BIG_ENDIAN
        }

    } // end namespace http
} // end namespace pstore

#endif // PSTORE_HTTP_ENDIAN_HPP
