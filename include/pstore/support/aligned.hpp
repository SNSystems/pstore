//*        _ _                      _  *
//*   __ _| (_) __ _ _ __   ___  __| | *
//*  / _` | | |/ _` | '_ \ / _ \/ _` | *
//* | (_| | | | (_| | | | |  __/ (_| | *
//*  \__,_|_|_|\__, |_| |_|\___|\__,_| *
//*            |___/                   *
//===- include/pstore/support/aligned.hpp ---------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file aligned.hpp
/// \brief Functions for aligning values and pointers.

#ifndef PSTORE_SUPPORT_ALIGNED_HPP
#define PSTORE_SUPPORT_ALIGNED_HPP

#include <cstdint>
#include <type_traits>

#include "pstore/support/assert.hpp"

namespace pstore {

    /// \returns True if the input value is a power of 2.
    template <typename Ty, typename = typename std::enable_if<std::is_unsigned<Ty>::value>>
    constexpr bool is_power_of_two (Ty const n) noexcept {
        //  if a number n is a power of 2 then bitwise & of n and n-1 will be zero.
        return n > 0U && !(n & (n - 1U));
    }

    /// \param v  The value to be aligned.
    /// \param align  The alignment required for \p v.
    /// \returns  The value closest to but greater than or equal to \p v for which
    /// \p v modulo \p align is zero.
    template <typename IntType, typename AlignType>
    constexpr IntType aligned (IntType const v, AlignType const align) noexcept {
        static_assert (std::is_unsigned<IntType>::value, "aligned() IntType must be unsigned");
        static_assert (std::is_unsigned<AlignType>::value, "aligned() AlignType must be unsigned");
        PSTORE_ASSERT (is_power_of_two (align));

        return (v + align - 1U) & static_cast<IntType> (~(align - 1U));
    }

    /// \param v  The value to be aligned.
    /// \returns  The value closest to but greater than or equal to \p v for which \p v modulo
    /// alignof(decltype(v)) is zero.
    template <typename AlignType, typename IntType,
              typename = std::enable_if<std::is_integral<IntType>::value>>
    constexpr IntType aligned (IntType const v) noexcept {
        return aligned (v, alignof (AlignType));
    }

    template <typename PointeeType>
    constexpr PointeeType const * aligned (void const * const p) noexcept {
        return reinterpret_cast<PointeeType const *> (
            aligned (reinterpret_cast<std::uintptr_t> (p), alignof (PointeeType)));
    }
    template <typename PointeeType>
    constexpr PointeeType * aligned (void * const p) noexcept {
        return reinterpret_cast<PointeeType *> (
            aligned (reinterpret_cast<std::uintptr_t> (p), alignof (PointeeType)));
    }

    template <typename DestPointeeType, typename SrcPointeeType = DestPointeeType>
    constexpr DestPointeeType const * aligned_ptr (SrcPointeeType const * const p) noexcept {
        return aligned<DestPointeeType> (reinterpret_cast<void const *> (p));
    }
    template <typename DestPointeeType, typename SrcPointeeType = DestPointeeType>
    constexpr DestPointeeType * aligned_ptr (SrcPointeeType * const p) noexcept {
        return aligned<DestPointeeType> (reinterpret_cast<void *> (p));
    }

    /// Calculate the value that must be added to p v in order that it has the alignment
    /// given by \p align.
    ///
    /// \param v      The value to be aligned.
    /// \param align  The alignment required for \p v.
    /// \returns  The value that must be added to \p v in order that it has the alignment given by
    /// \p align.
    template <typename Ty>
    constexpr Ty calc_alignment (Ty const v, std::size_t const align) noexcept {
        PSTORE_ASSERT (is_power_of_two (align));
        return (align == 0U) ? 0U : ((v + align - 1U) & ~(align - 1U)) - v;
    }

    /// Calculate the value that must be added to \p v in order for it to have the alignment
    /// required by type Ty.
    ///
    /// \param v  The value to be aligned.
    /// \returns  The value that must be added to \p v in order that it has the alignment required
    /// by type Ty.
    template <typename Ty>
    constexpr Ty calc_alignment (Ty const v) noexcept {
        return calc_alignment (v, alignof (Ty));
    }

} // end namespace pstore

#endif // PSTORE_SUPPORT_ALIGNED_HPP
