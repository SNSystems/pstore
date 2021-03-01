//===- include/pstore/core/crc32.hpp ----------------------*- mode: C++ -*-===//
//*                _________   *
//*   ___ _ __ ___|___ /___ \  *
//*  / __| '__/ __| |_ \ __) | *
//* | (__| | | (__ ___) / __/  *
//*  \___|_|  \___|____/_____| *
//*                            *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file crc32.hpp

#ifndef PSTORE_CORE_CRC32_HPP
#define PSTORE_CORE_CRC32_HPP

#include <array>
#include <cstdint>
#include <cstdlib>

namespace pstore {

    namespace details {
        extern std::array<std::uint32_t, 256> const crc32_tab;
    } // end namespace details

    template <typename SpanType>
    std::uint32_t crc32 (SpanType buf) noexcept {
        auto * p = reinterpret_cast<std::uint8_t const *> (buf.data ());
        auto crc = std::uint32_t{0};
        auto size = buf.size_bytes ();
        while (size--) {
            crc = details::crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
        }
        return crc ^ ~0U;
    }

} // end namespace pstore
#endif // PSTORE_CORE_CRC32_HPP
