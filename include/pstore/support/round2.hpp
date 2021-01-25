//*                            _ ____   *
//*  _ __ ___  _   _ _ __   __| |___ \  *
//* | '__/ _ \| | | | '_ \ / _` | __) | *
//* | | | (_) | |_| | | | | (_| |/ __/  *
//* |_|  \___/ \__,_|_| |_|\__,_|_____| *
//*                                     *
//===- include/pstore/support/round2.hpp ----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_SUPPORT_ROUND2_HPP
#define PSTORE_SUPPORT_ROUND2_HPP

#include <cstdint>

namespace pstore {

    // The implementation of these functions is based on code published in the "Bit Twiddling Hacks"
    // web page by Sean Eron Anderson (seander@cs.stanford.edu) found at
    // <https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2>. The original code is
    // public domain.


    inline std::uint8_t round_to_power_of_2 (std::uint8_t v) noexcept {
        --v;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        ++v;
        return v;
    }

    inline std::uint16_t round_to_power_of_2 (std::uint16_t v) noexcept {
        --v;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        ++v;
        return v;
    }

    inline std::uint32_t round_to_power_of_2 (std::uint32_t v) noexcept {
        --v;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        ++v;
        return v;
    }

    inline std::uint64_t round_to_power_of_2 (std::uint64_t v) noexcept {
        --v;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        ++v;
        return v;
    }

} // end namespace pstore

#endif // PSTORE_SUPPORT_ROUND2_HPP
