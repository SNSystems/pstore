//*               _               *
//* __      _____| | _____ _   _  *
//* \ \ /\ / / __| |/ / _ \ | | | *
//*  \ V  V /\__ \   <  __/ |_| | *
//*   \_/\_/ |___/_|\_\___|\__, | *
//*                        |___/  *
//===- lib/http/wskey.cpp -------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file wskey.cpp
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
#include "pstore/http/wskey.hpp"

#include <array>
#include <cctype>

#include "pstore/support/array_elements.hpp"
#include "pstore/support/base64.hpp"

namespace pstore {
    namespace http {

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
                digest[i] = static_cast<std::uint8_t> (intermediate_hash_[i >> 2U] >>
                                                       8U * (3U - (i & 0x03U)));
            }
            return digest;
        }

        // process message block
        // ~~~~~~~~~~~~~~~~~~~~~
        void sha1::process_message_block () noexcept {
            // Constants defined in SHA-1.
            static constexpr std::uint32_t k[] = {UINT32_C (0x5A827999), UINT32_C (0x6ED9EBA1),
                                                  UINT32_C (0x8F1BBCDC), UINT32_C (0xCA62C1D6)};
            std::array<std::uint32_t, 80> w; // Word sequence.

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

        // pad message
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
                message_block_[index_] = 0x80;
                ++index_;
                for (; index_ < 56; ++index_) {
                    message_block_[index_] = 0;
                }
            }

            // Store the message length as the last 8 octets.
            message_block_[56] = static_cast<std::uint8_t> (length_ >> 56U);
            message_block_[57] = static_cast<std::uint8_t> (length_ >> 48U);
            message_block_[58] = static_cast<std::uint8_t> (length_ >> 40U);
            message_block_[59] = static_cast<std::uint8_t> (length_ >> 32U);
            message_block_[60] = static_cast<std::uint8_t> (length_ >> 24U);
            message_block_[61] = static_cast<std::uint8_t> (length_ >> 16U);
            message_block_[62] = static_cast<std::uint8_t> (length_ >> 8U);
            message_block_[63] = static_cast<std::uint8_t> (length_);

            this->process_message_block ();
        }

        // digest to base64 [static]
        // ~~~~~~~~~~~~~~~~
        std::string sha1::digest_to_base64 (sha1::result_type const & digest) {
            std::string result;
            to_base64 (std::begin (digest), std::end (digest), std::back_inserter (result));
            return result;
        }


        // source key
        // ~~~~~~~~~~
        std::string source_key (std::string const & k) {
            sha1 hash;

            auto first = std::string::size_type{0};
            auto const length = k.length ();
            while (first < length && std::isspace (static_cast<unsigned char> (k[first]))) {
                ++first;
            }
            auto last = length;
            while (last > 0 && std::isspace (static_cast<unsigned char> (k[last - 1]))) {
                --last;
            }

            hash.input (gsl::make_span (reinterpret_cast<std::uint8_t const *> (&k[first]),
                                        reinterpret_cast<std::uint8_t const *> (&k[last])));

            static char const magic[] = {'2', '5', '8', 'E', 'A', 'F', 'A', '5', '-',
                                         'E', '9', '1', '4', '-', '4', '7', 'D', 'A',
                                         '-', '9', '5', 'C', 'A', '-', 'C', '5', 'A',
                                         'B', '0', 'D', 'C', '8', '5', 'B', '1', '1'};
            hash.input (gsl::make_span (reinterpret_cast<std::uint8_t const *> (magic),
                                        array_elements (magic)));

            return sha1::digest_to_base64 (hash.result ());
        }

    } // end namespace http
} // end namespace pstore
