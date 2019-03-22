//*      _           _  *
//*  ___| |__   __ _/ | *
//* / __| '_ \ / _` | | *
//* \__ \ | | | (_| | | *
//* |___/_| |_|\__,_|_| *
//*                     *
//===- lib/httpd/sha1.cpp -------------------------------------------------===//
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
/*
 *  sha1.c
 *
 *  Description:
 *      This file implements the Secure Hashing Algorithm 1 as
 *      defined in FIPS PUB 180-1 published April 17, 1995.
 *
 *      The SHA-1, produces a 160-bit message digest for a given
 *      data stream.  It should take about 2**n steps to find a
 *      message with the same digest as a given message and
 *      2**(n/2) to find any two messages with the same digest,
 *      when n is the digest size in bits.  Therefore, this
 *      algorithm can serve as a means of providing a
 *      "fingerprint" for a message.
 *
 *  Portability Issues:
 *      SHA-1 is defined in terms of 32-bit "words".  This code
 *      uses <stdint.h> (included via "sha1.h" to define 32 and 8
 *      bit unsigned integer types.  If your C compiler does not
 *      support 32 bit unsigned integers, this code is not
 *      appropriate.
 *
 *  Caveats:
 *      SHA-1 is designed to work with messages less than 2^64 bits
 *      long.  Although SHA-1 allows a message digest to be generated
 *      for messages of any number of bits less than 2^64, this
 *      implementation only works with messages with a length that is
 *      a multiple of the size of an 8-bit character.
 *
 */
#include "pstore/httpd/sha1.hpp"

namespace pstore {
    namespace httpd {

        constexpr std::array<std::uint32_t, sha1::hash_size / 4> sha1::initial_intermediate;

        // input
        // ~~~~~
        sha1 & sha1::input (gsl::span<std::uint8_t const> const & span) noexcept {
            if (span.data () == nullptr || computed_ || corrupted_) {
                return *this;
            }

            for (std::uint8_t const b : span) {
                message_block_[index_] = b;
                ++index_;

                auto const old_length = length_;
                length_ += 8;
                if (length_ < old_length) { // Detect overflow.
                    corrupted_ = true;      // Message is too long.
                    break;
                }

                if (index_ == 64) {
                    this->process_message_block ();
                }
            }
            return *this;
        }

        // result
        // ~~~~~~
        auto sha1::result () noexcept -> result_type {
            if (!computed_) {
                this->pad_message ();
                // Message may be sensitive, clear it out.
                std::fill (std::begin (message_block_), std::end (message_block_), std::uint8_t{0});
                length_ = 0U; // and clear length.
                computed_ = true;
            }

            result_type digest;
            for (auto i = 0U; i < hash_size; ++i) {
                digest[i] =
                    static_cast<std::uint8_t> (intermediate_hash_[i >> 2] >> 8 * (3 - (i & 0x03)));
            }
            return digest;
        }

        // process_message_block
        // ~~~~~~~~~~~~~~~~~~~~~
        void sha1::process_message_block () noexcept {
            // Constants defined in SHA-1.
            static constexpr std::uint32_t k[] = {0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6};
            std::uint32_t w[80]; // Word sequence.

            for (auto t = 0U; t < 16U; ++t) {
                w[t] = static_cast<std::uint32_t> (message_block_[t * 4U] << 24U);
                w[t] |= static_cast<std::uint32_t> (message_block_[t * 4U + 1U] << 16U);
                w[t] |= static_cast<std::uint32_t> (message_block_[t * 4U + 2U] << 8U);
                w[t] |= static_cast<std::uint32_t> (message_block_[t * 4U + 3U]);
            }
            for (auto t = 16U; t < 80U; ++t) {
                w[t] = circular_shift (1, w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16]);
            }

            std::uint32_t a = intermediate_hash_[0];
            std::uint32_t b = intermediate_hash_[1];
            std::uint32_t c = intermediate_hash_[2];
            std::uint32_t d = intermediate_hash_[3];
            std::uint32_t e = intermediate_hash_[4];

            for (auto t = 0U; t < 20U; ++t) {
                std::uint32_t const temp =
                    circular_shift (5, a) + ((b & c) | ((~b) & d)) + e + w[t] + k[0];
                e = d;
                d = c;
                c = circular_shift (30, b);
                b = a;
                a = temp;
            }

            for (auto t = 20U; t < 40U; ++t) {
                std::uint32_t const temp = circular_shift (5, a) + (b ^ c ^ d) + e + w[t] + k[1];
                e = d;
                d = c;
                c = circular_shift (30, b);
                b = a;
                a = temp;
            }

            for (auto t = 40U; t < 60U; ++t) {
                std::uint32_t const temp =
                    circular_shift (5, a) + ((b & c) | (b & d) | (c & d)) + e + w[t] + k[2];
                e = d;
                d = c;
                c = circular_shift (30, b);
                b = a;
                a = temp;
            }

            for (auto t = 60U; t < 80U; ++t) {
                std::uint32_t const temp = circular_shift (5, a) + (b ^ c ^ d) + e + w[t] + k[3];
                e = d;
                d = c;
                c = circular_shift (30, b);
                b = a;
                a = temp;
            }

            intermediate_hash_[0] += a;
            intermediate_hash_[1] += b;
            intermediate_hash_[2] += c;
            intermediate_hash_[3] += d;
            intermediate_hash_[4] += e;

            index_ = 0;
        }

        // pad_message
        // ~~~~~~~~~~~
        void sha1::pad_message () noexcept {
            // Note that many of the variable names in this code, especially the single character
            // names, were used because those were the names used in the publication.

            // Check to see if the current message block is too small to hold the initial padding
            // bits and length.  If so, we will pad the block, process it, and then continue padding
            // into a second block.
            if (index_ > 55) {
                message_block_[index_] = 0x80;
                ++index_;

                for (; index_ < 64; ++index_) {
                    message_block_[index_] = 0;
                }

                this->process_message_block ();

                for (; index_ < 56; ++index_) {
                    message_block_[index_] = 0;
                }
            } else {
                message_block_[index_++] = 0x80;
                for (; index_ < 56; ++index_) {
                    message_block_[index_] = 0;
                }
            }

            // Store the message length as the last 8 octets.
            message_block_[56] = static_cast<std::uint8_t> (length_ >> 56);
            message_block_[57] = static_cast<std::uint8_t> (length_ >> 48);
            message_block_[58] = static_cast<std::uint8_t> (length_ >> 40);
            message_block_[59] = static_cast<std::uint8_t> (length_ >> 32);
            message_block_[60] = static_cast<std::uint8_t> (length_ >> 24);
            message_block_[61] = static_cast<std::uint8_t> (length_ >> 16);
            message_block_[62] = static_cast<std::uint8_t> (length_ >> 8);
            message_block_[63] = static_cast<std::uint8_t> (length_);

            this->process_message_block ();
        }

        // digest_to_base64 [static]
        // ~~~~~~~~~~~~~~~~
        std::string sha1::digest_to_base64 (sha1::result_type const & digest) {
            char const * alphabet =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string result;
            for (auto ctr = 0U; ctr < 18U; ctr += 3U) {
                result += alphabet[(digest[ctr] >> 2U) & 0x3F];
                result +=
                    alphabet[((digest[ctr] & 0x03) << 4U) | ((digest[ctr + 1U] & 0xF0) >> 4U)];
                result +=
                    alphabet[((digest[ctr + 1U] & 0x0F) << 2U) | ((digest[ctr + 2U] & 0xC0) >> 6U)];
                result += alphabet[digest[ctr + 2U] & 0x3F];
            }
            result += alphabet[(digest[18] >> 2U) & 0x3F];
            result += alphabet[((digest[18] & 0x03) << 4U) | ((digest[19] & 0xF0) >> 4U)];
            result += alphabet[(digest[19] & 0x0F) << 2U];
            result += '=';
            return result;
        }

    } // end namespace httpd
} // end namespace pstore
