//*  _                     __   _  _    *
//* | |__   __ _ ___  ___ / /_ | || |   *
//* | '_ \ / _` / __|/ _ \ '_ \| || |_  *
//* | |_) | (_| \__ \  __/ (_) |__   _| *
//* |_.__/ \__,_|___/\___|\___/   |_|   *
//*                                     *
//===- include/pstore/support/base64.hpp ----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_SUPPORT_BASE64_HPP
#define PSTORE_SUPPORT_BASE64_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "pstore/support/gsl.hpp"
#include "pstore/support/maybe.hpp"

namespace pstore {

    /// Converts an array of bytes to ASCII85.
    ///
    /// \tparam ForwardIterator  A LegacyForwardIterator iterator type.
    /// \tparam OutputIterator  A LegacyOutputIterator iterator type.
    /// \param first  The first in the range of elements to be converted.
    /// \param last  The end of the range of elements to be converted.
    /// \param out  The output iterator to which the encoded characters are written.
    /// \returns Output iterator to the element in the destination range, one past the last element
    ///   copied.
    template <typename ForwardIterator, typename OutputIterator>
    OutputIterator to_base64 (ForwardIterator first, ForwardIterator last, OutputIterator out) {
        static constexpr std::array<char, 64> alphabet{
            {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
             'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
             'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
             'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'}};
        auto const size = std::distance (first, last);
        // Consume the input, converting three input bytes to four output characters.
        for (auto index = decltype (size){0}; index < size / 3; ++index) {
            auto value1 = static_cast<std::uint32_t> (*(first++) << 16);
            value1 += static_cast<std::uint32_t> (*(first++) << 8);
            value1 += static_cast<std::uint32_t> (*(first++));

            *(out++) = alphabet[(value1 & 0x00FC0000) >> 18];
            *(out++) = alphabet[(value1 & 0x0003F000) >> 12];
            *(out++) = alphabet[(value1 & 0x00000FC0) >> 6];
            *(out++) = alphabet[value1 & 0x0000003F];
        }

        // Deal with the remaining 0..2 characters.
        switch (size % 3) {
        case 0: break;
        case 1: {
            auto const value2 = static_cast<std::uint32_t> (*(first++) << 16);

            *(out++) = alphabet[(value2 & 0x00FC0000) >> 18];
            *(out++) = alphabet[(value2 & 0x0003F000) >> 12];
            *(out++) = '=';
            *(out++) = '=';
        } break;
        case 2: {
            auto value3 = static_cast<std::uint32_t> (*(first++) << 16);
            value3 += static_cast<std::uint32_t> (*(first++) << 8);

            *(out++) = alphabet[(value3 & 0x00FC0000) >> 18];
            *(out++) = alphabet[(value3 & 0x0003F000) >> 12];
            *(out++) = alphabet[(value3 & 0x00000FC0) >> 6];
            *(out++) = '=';
        } break;
        default:
            PSTORE_ASSERT (false); //! OCLINT(PH - don't warn about a conditional constant)
            break;
        }
        PSTORE_ASSERT (first == last);
        return out;
    }

    namespace details {

        template <typename OutputIterator>
        OutputIterator decode4 (std::array<std::uint8_t, 4> const & in, OutputIterator out,
                                unsigned const count) {
            std::array<std::uint8_t, 3> result;
            result[0] = static_cast<std::uint8_t> (static_cast<std::uint8_t> (in[0] << 2) +
                                                   ((in[1] & 0x30U) >> 4));
            result[1] = static_cast<std::uint8_t> (((in[1] & 0xFU) << 4) + ((in[2] & 0x3CU) >> 2));
            result[2] = static_cast<std::uint8_t> (((in[2] & 0x3U) << 6) + in[3]);
            for (auto ctr = 0U; ctr < count; ++ctr) {
                *out = result[ctr];
                ++out;
            }
            return out;
        }

    } // end namespace details

    template <typename InputIterator, typename OutputIterator>
    maybe<OutputIterator> from_base64 (InputIterator first, InputIterator last,
                                       OutputIterator out) {
        auto count = 0U;
        std::array<std::uint8_t, 4> buff;

        for (; first != last; ++first) {
            auto c = *first;
            if (c >= 'A' && c <= 'Z') {
                c = c - 'A';
            } else if (c >= 'a' && c <= 'z') {
                c = c - 'a' + 26;
            } else if (c >= '0' && c <= '9') {
                c = c - '0' + 26 * 2;
            } else if (c == '+') {
                c = 0x3E;
            } else if (c == '/') {
                c = 0x3F;
            } else if (c == '=') {
                continue;
            } else {
                break; // error.
            }
            buff[count++] = static_cast<std::uint8_t> (c);
            if (count >= 4U) {
                out = details::decode4 (buff, out, 3);
                count = 0U;
            }
        }
        if (count > 0) {
            std::fill (buff.begin () + count, buff.end (), std::uint8_t{0});
            out = details::decode4 (buff, out, count - 1);
        }
        return first == last ? just (out) : nothing<OutputIterator> ();
    }


} // end namespace pstore

#endif // PSTORE_SUPPORT_BASE64_HPP
