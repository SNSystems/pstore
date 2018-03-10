//*                _________   *
//*   ___ _ __ ___|___ /___ \  *
//*  / __| '__/ __| |_ \ __) | *
//* | (__| | | (__ ___) / __/  *
//*  \___|_|  \___|____/_____| *
//*                            *
//===- include/pstore/core/crc32.hpp --------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
/// \file crc32.hpp

#ifndef PSTORE_CRC32_HPP
#define PSTORE_CRC32_HPP (1)

#include <array>
#include <cstdint>
#include <cstdlib>

namespace pstore {

    namespace details {
        extern std::array<std::uint32_t, 256> const crc32_tab;
    } // end namespace details

    template <typename SpanType>
    std::uint32_t crc32 (SpanType buf) noexcept {
        auto p = reinterpret_cast<std::uint8_t const *> (buf.data ());
        auto crc = std::uint32_t{0};
        auto size = buf.size_bytes ();
        while (size--) {
            crc = details::crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
        }
        return crc ^ ~0U;
    }

} // end namespace pstore
#endif // PSTORE_CRC32_HPP
// eof: include/pstore/core/crc32.hpp
