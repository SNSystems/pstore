//*      _           _  *
//*  ___| |__   __ _/ | *
//* / __| '_ \ / _` | | *
//* \__ \ | | | (_| | | *
//* |___/_| |_|\__,_|_| *
//*                     *
//===- include/pstore/httpd/sha1.hpp --------------------------------------===//
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
/// This is the header file for code which implements the Secure Hashing Algorithm 1 as defined in
/// FIPS PUB 180-1 published April 17, 1995.
///
/// Many of the variable names in this code, especially the single character names, were used
/// because those were the names used in the publication.
#ifndef PSTORE_HTTPD_SHA1_HPP
#define PSTORE_HTTPD_SHA1_HPP

#include <array>
#include <cstdint>
#include <cstdlib>

#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace httpd {

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
            /// in the 19th element. \returns The SHA1 hash digest.
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

            /// Processes the next 512 bits of the message stored in the Message_Block array.
            void process_message_block () noexcept;

            /// According to the standard, the message must be padded to an even 512 bits.  The
            /// first padding bit must be a '1'.  The last 64 bits represent the length of the
            /// original message. All bits in between should be 0.  This function will pad the
            /// message according to those rules by filling the message_block_ array accordingly. It
            /// will also call process_message_block(). When it returns, it can be assumed that the
            /// message digest has been computed.
            void pad_message () noexcept;

            // The SHA1 circular left shift.
            static inline constexpr std::uint32_t circular_shift (unsigned bits,
                                                                  std::uint32_t word) {
                return (word << bits) | (word >> (32U - bits));
            }

            static constexpr std::array<std::uint32_t, hash_size / 4> initial_intermediate = {
                {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0}};
        };


    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTPD_SHA1_HPP