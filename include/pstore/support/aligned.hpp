//*        _ _                      _  *
//*   __ _| (_) __ _ _ __   ___  __| | *
//*  / _` | | |/ _` | '_ \ / _ \/ _` | *
//* | (_| | | | (_| | | | |  __/ (_| | *
//*  \__,_|_|_|\__, |_| |_|\___|\__,_| *
//*            |___/                   *
//===- include/pstore/support/aligned.hpp ---------------------------------===//
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
#ifndef PSTORE_SUPPORT_ALIGNED_HPP
#define PSTORE_SUPPORT_ALIGNED_HPP

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace pstore {

    /// \returns True if the input value is a power of 2.
    template <typename Ty, typename = typename std::enable_if<std::is_unsigned<Ty>::value>>
    inline constexpr bool is_power_of_two (Ty n) noexcept {
        //  if a number n is a power of 2 then bitwise & of n and n-1 will be zero.
        return n > 0U && !(n & (n - 1U));
    }

    /// \param v  The value to be aligned.
    /// \param align  The alignment required for \p v.
    /// \returns  The value closest to but greater than or equal to \p v for which
    /// \p v modulo \p align is zero.
    template <typename IntType, typename AlignType>
    inline IntType aligned (IntType v, AlignType align) noexcept {
        static_assert (std::is_unsigned<IntType>::value, "aligned() IntType must be unsigned");
        static_assert (std::is_unsigned<AlignType>::value, "aligned() AlignType must be unsigned");
        assert (is_power_of_two (align));

        return (v + align - 1U) & static_cast<IntType> (~(align - 1U));
    }

    /// \param v  The value to be aligned.
    /// \returns  The value closest to but greater than or equal to \p v for which \p v modulo
    /// alignof(decltype(v)) is zero.
    template <typename AlignType, typename IntType,
              typename = std::enable_if<std::is_integral<IntType>::value>>
    inline IntType aligned (IntType v) noexcept {
        return aligned (v, alignof (AlignType));
    }

    template <typename PointeeType>
    inline PointeeType const * aligned (void const * p) noexcept {
        return reinterpret_cast<PointeeType const *> (
            aligned (reinterpret_cast<std::uintptr_t> (p), alignof (PointeeType)));
    }
    template <typename PointeeType>
    inline PointeeType * aligned (void * p) noexcept {
        return reinterpret_cast<PointeeType *> (
            aligned (reinterpret_cast<std::uintptr_t> (p), alignof (PointeeType)));
    }

    template <typename DestPointeeType, typename SrcPointeeType = DestPointeeType>
    inline DestPointeeType * aligned_ptr (SrcPointeeType * p) noexcept {
        return aligned<DestPointeeType> (reinterpret_cast<void *> (p));
    }
    template <typename DestPointeeType, typename SrcPointeeType = DestPointeeType>
    inline DestPointeeType const * aligned_ptr (SrcPointeeType const * p) noexcept {
        return aligned<DestPointeeType> (reinterpret_cast<void const *> (p));
    }

    /// Calculate the value that must be added to p v in order that it has the alignment
    /// given by \p align.
    ///
    /// \param v      The value to be aligned.
    /// \param align  The alignment required for \p v.
    /// \returns  The value that must be added to \p v in order that it has the alignment given by
    /// \p align.
    template <typename Ty>
    inline Ty calc_alignment (Ty v, std::size_t align) noexcept {
        assert (is_power_of_two (align));
        return (align == 0U) ? 0U : ((v + align - 1U) & ~(align - 1U)) - v;
    }

    /// Calculate the value that must be added to \p v in order for it to have the alignment
    /// required by type Ty.
    ///
    /// \param v  The value to be aligned.
    /// \returns  The value that must be added to \p v in order that it has the alignment required
    /// by type Ty.
    template <typename Ty>
    inline Ty calc_alignment (Ty v) noexcept {
        return calc_alignment (v, alignof (Ty));
    }

} // end namespace pstore

#endif // PSTORE_SUPPORT_ALIGNED_HPP
