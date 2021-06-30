//===- include/pstore/http/wskey.hpp ----------------------*- mode: C++ -*-===//
//*               _               *
//* __      _____| | _____ _   _  *
//* \ \ /\ / / __| |/ / _ \ | | | *
//*  \ V  V /\__ \   <  __/ |_| | *
//*   \_/\_/ |___/_|\_\___|\__, | *
//*                        |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file wskey.hpp
/// \brief Code to generate the value of the Sec-WebSocket-Accept header
///
/// This is the header file for code which implements the Secure Hashing Algorithm 1 as defined in
/// FIPS PUB 180-1 published April 17, 1995.
///
/// Many of the variable names in this code, especially the single character names, were used
/// because those were the names used in the publication.
#ifndef PSTORE_HTTP_WSKEY_HPP
#define PSTORE_HTTP_WSKEY_HPP

#include <array>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace http {

        class sha1 {
        public:
            static constexpr unsigned hash_size = 20U;
            using result_type = std::array<std::uint8_t, hash_size>;

            /// This function accepts an array of octets as the next portion of the message.
            /// \param span  A span of bytes representing the next portion of the message.
            /// \returns *this
            sha1 & input (gsl::span<std::uint8_t const> const & span) noexcept;

            /// This function will return the 160-bit message digest.
            /// \note The first octet of hash is stored in the 0th element, the last octet of hash
            /// in the 19th element.
            /// \returns The SHA1 hash digest.
            result_type result () noexcept;

            static std::string digest_to_base64 (sha1::result_type const & digest);

        private:
            std::array<std::uint32_t, hash_size / 4> intermediate_hash_ =
                initial_intermediate; ///< Message Digest.

            std::uint64_t length_ = 0U;                  ///< Message length in bits.
            unsigned index_ = 0U;                        ///< Index into message block array.
            std::array<std::uint8_t, 64> message_block_; ///< 512-bit message blocks.

            bool computed_ = false;  // Is the digest computed?
            bool corrupted_ = false; // Is the message digest corrupted?

            /// Processes the next 512 bits of the message stored in the message_block_ array.
            void process_message_block () noexcept;

            /// According to the standard, the message must be padded to an even 512 bits. The
            /// first padding bit must be a '1'. The last 64 bits represent the length of the
            /// original message. All bits in between should be 0. This function will pad the
            /// message according to those rules by filling the message_block_ array accordingly. It
            /// will also call process_message_block(). When it returns, it can be assumed that the
            /// message digest has been computed.
            void pad_message () noexcept;

            // The SHA1 circular left shift.
            static constexpr std::uint32_t circular_shift (unsigned const bits,
                                                           std::uint32_t const word) noexcept {
                return (word << bits) | (word >> (32U - bits));
            }

            static constexpr std::array<std::uint32_t, hash_size / 4> initial_intermediate = {
                {UINT32_C (0x67452301), UINT32_C (0xEFCDAB89), UINT32_C (0x98BADCFE),
                 UINT32_C (0x10325476), UINT32_C (0xC3D2E1F0)}};
        };

        std::string source_key (std::string const & k);

    } // end namespace http
} // end namespace pstore

#endif // PSTORE_HTTP_WSKEY_HPP
