//*  _                    _________   *
//* | |__   __ _ ___  ___|___ /___ \  *
//* | '_ \ / _` / __|/ _ \ |_ \ __) | *
//* | |_) | (_| \__ \  __/___) / __/  *
//* |_.__/ \__,_|___/\___|____/_____| *
//*                                   *
//===- lib/core/base32.hpp ------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file base32.hpp

#ifndef PSTORE_CORE_BASE32_HPP
#define PSTORE_CORE_BASE32_HPP

#include <algorithm>
#include <cmath>

#include "pstore/support/uint128.hpp"

namespace pstore {
    namespace base32 {


        extern std::array<char const, 32> const alphabet;

        /// \brief Converts an unsigned integer value to a sequence of base-32 characters.
        /// The encoding conforms to RFC4648 but does not employ any padding characters. Note that
        /// the resulting output has the least significant digit first.
        template <typename IntType, typename OutputIterator>
        OutputIterator convert (IntType val, OutputIterator out) {
            PSTORE_STATIC_ASSERT (std::is_unsigned<IntType>::value);
            constexpr auto mask = (1U << 5) - 1U;
            PSTORE_ASSERT (mask == alphabet.size () - 1U);
            do {
                *(out++) = alphabet[val & mask];
            } while (val >>= 5);
            return out;
        }

        template <typename OutputIterator>
        OutputIterator convert (uint128 const wide, OutputIterator out) {
            auto high = wide.high ();
            auto low = wide.low ();
            constexpr auto mask = (1U << 5) - 1U;
            PSTORE_ASSERT (mask == alphabet.size () - 1U);
            do {
                *(out++) = alphabet[low & mask];
                low >>= 5;
                low |= (high & mask) << (64 - 5);
                high >>= 5;
            } while ((low | high) != 0);
            return out;
        }

        /// Converts an unsigned integer value (which may be uint128) to a std::string instance
        /// containing a sequence of base-32 characters. Note that the resulting output has the
        /// least significant digit first.
        template <typename IntType>
        std::string convert (IntType val) {
            std::string result;
            auto const max_length = 26;
            PSTORE_ASSERT (std::pow (32.0, max_length) >= std::pow (2.0, 128));
            result.reserve (max_length);
            convert (val, std::back_inserter (result));
            return result;
        }

    } // namespace base32
} // namespace pstore

#endif // PSTORE_CORE_BASE32_HPP
