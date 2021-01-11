//*  _                    _________   *
//* | |__   __ _ ___  ___|___ /___ \  *
//* | '_ \ / _` / __|/ _ \ |_ \ __) | *
//* | |_) | (_| \__ \  __/___) / __/  *
//* |_.__/ \__,_|___/\___|____/_____| *
//*                                   *
//===- lib/core/base32.hpp ------------------------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
