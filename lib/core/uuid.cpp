//*              _     _  *
//*  _   _ _   _(_) __| | *
//* | | | | | | | |/ _` | *
//* | |_| | |_| | | (_| | *
//*  \__,_|\__,_|_|\__,_| *
//*                       *
//===- lib/core/uuid.cpp --------------------------------------------------===//
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

/// \file uuid.cpp
/// \brief Implements a class which provides RFC4122 UUIDs.

#include "pstore/core/uuid.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <iomanip>
#include <iterator>
#include <ostream>

#include "pstore/support/error.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/random.hpp"

namespace {
    char digit_to_hex (unsigned v) {
        assert (v < 0x10);
        return static_cast<char> (v + ((v < 10) ? '0' : 'a' - 10));
    }


    template <typename OutputIterator>
    class hex_decoder {
    public:
        explicit hex_decoder (OutputIterator out)
                : out_{out} {}

        OutputIterator append (unsigned value) {
            assert (value < 16U);
            if (is_high_) {
                *out_ = static_cast<std::uint8_t> (value << 4);
            } else {
                *(out_++) |= value;
            }
            is_high_ = !is_high_;
            return out_;
        }

        bool is_high () const { return is_high_; }

    private:
        OutputIterator out_;
        bool is_high_ = true;
    };

    template <typename OutputIterator>
    auto make_hex_decoder (OutputIterator out) -> hex_decoder<OutputIterator> {
        return hex_decoder<OutputIterator> (out);
    }
} // end anonymous namespace

namespace pstore {

    // (ctor)
    // ~~~~~~
    uuid::uuid () {
        static random_generator<unsigned> random;
        auto generator = []() {
            constexpr auto max = std::numeric_limits<std::uint8_t>::max ();
            return static_cast<std::uint8_t> (random.get (max));
        };
        std::generate (std::begin (data_), std::end (data_), generator);

        // Set variant: must be 0b10xxxxxx
        data_[variant_octet] &= 0xBF; // 0b10111111;
        data_[variant_octet] |= 0x80; // 0b10000000;

        // Set version: must be 0b0100xxxx
        data_[version_octet] &= 0x4F; // 0b01001111;
        data_[version_octet] |= static_cast<std::uint8_t> (version_type::random_number_based) << 4;

        assert (this->variant () == uuid::variant_type::rfc_4122);
        assert (this->version () == uuid::version_type::random_number_based);
    }

#ifdef _WIN32
    uuid::uuid (UUID const & u) {
        // UUID octets are in network byte order, but the Microsoft API
        // struct has them encoded in native types. That means that I have to
        // extract the bytes one at a time.
        auto out = std::begin (data_);
        *(out++) = get_byte (u.Data1, 3);
        *(out++) = get_byte (u.Data1, 2);
        *(out++) = get_byte (u.Data1, 1);
        *(out++) = get_byte (u.Data1, 0);

        *(out++) = get_byte (u.Data2, 1);
        *(out++) = get_byte (u.Data2, 0);

        *(out++) = get_byte (u.Data3, 1);
        *(out++) = get_byte (u.Data3, 0);

        // Guarantee that we'll interpret the fields of the UUID struct as 16 bytes.
        assert (std::distance (std::begin (data_), out) +
                    std::distance (std::begin (u.Data4), std::end (u.Data4)) ==
                elements);
        PSTORE_STATIC_ASSERT (sizeof (u.Data4[0]) == sizeof (std::uint8_t));
        std::copy (std::begin (u.Data4), std::end (u.Data4), out);
    }
#elif defined(__APPLE__)
    uuid::uuid (uuid_t const & bytes) {
        PSTORE_STATIC_ASSERT (sizeof (bytes[0]) == sizeof (value_type));
        PSTORE_STATIC_ASSERT (sizeof (bytes) / sizeof (bytes[0]) == elements);
        std::copy (std::begin (bytes), std::end (bytes), data_.begin ());
    }
#endif

    // Construct from the canonical UUID string representation as defined by RFC4122.
    uuid::uuid (std::string const & str) {
        maybe<uuid> d = uuid::from_string (str);
        if (!d.has_value ()) {
            raise (pstore::error_code::uuid_parse_error);
        }
        data_ = d->array ();
    }

    // from_string
    // ~~~~~~~~~~~
    maybe<uuid> uuid::from_string (std::string const & str) {
        if (str.length () != string_length) {
            return nothing<uuid> ();
        }

        container_type data;
        auto out = make_hex_decoder (std::begin (data));
        auto count = 0U;
        for (auto digit : str) {
            switch (count++) {
            case 8:
            case 13:
            case 18:
            case 23:
                if (digit != '-') {
                    return nothing<uuid> ();
                }
                assert (out.is_high ());
                break;
            default:
                if (digit >= 'a' && digit <= 'f') {
                    out.append (static_cast<unsigned> (digit) - ('a' - 10));
                } else if (digit >= 'A' && digit <= 'F') {
                    out.append (static_cast<unsigned> (digit) - ('A' - 10));
                } else if (digit >= '0' && digit <= '9') {
                    out.append (static_cast<unsigned> (digit) - '0');
                } else {
                    return nothing<uuid> ();
                }
                break;
            }
        }

        return just (uuid{data});
    }

    // variant
    // ~~~~~~~
    auto uuid::variant () const noexcept -> variant_type {
        auto const octet = data_[variant_octet];
        if ((octet & 0x80 /*0b10000000*/) == 0x00 /*0b00000000*/) { // 0b0xxxxxxx
            return variant_type::ncs;
        }
        if ((octet & 0xC0 /*0b11000000*/) == 0x80 /*0b10000000*/) { // 0b10xxxxxx
            return variant_type::rfc_4122;
        }
        if ((octet & 0xE0 /*0b11100000*/) == 0xC0 /*0b11000000*/) { // 0b110xxxxx
            return variant_type::microsoft;
        }
        assert ((octet & 0xE0 /*0b11100000*/) == 0xE0 /*0b11100000*/); // 0b111xxxx
        return variant_type::future;
    }

    // version
    // ~~~~~~~
    auto uuid::version () const noexcept -> version_type {
        switch (data_[version_octet] & 0xF0) {
        case 0x10: return version_type::time_based;
        case 0x20: return version_type::dce_security;
        case 0x30: return version_type::name_based_md5;
        case 0x40: return version_type::random_number_based;
        case 0x50: return version_type::name_based_sha1;
        default: return version_type::unknown;
        }
    }

    // is_null
    // ~~~~~~~
    bool uuid::is_null () const noexcept {
        for (auto v : data_) {
            if (v) {
                return false;
            }
        }
        return true;
    }

    // str
    // ~~~
    std::string uuid::str () const {
        std::string resl;
        resl.reserve (string_length);
        auto index = 0U;
        std::for_each (std::begin (data_), std::end (data_), [&](unsigned c) {
            switch (index++) {
            case 4:
            case 6:
            case 8:
            case 10: resl += '-'; PSTORE_FALLTHROUGH;
            default: resl += digit_to_hex ((c >> 4) & 0x0F); resl += digit_to_hex (c & 0x0F);
            }
        });
        return resl;
    }

    // operator<<
    // ~~~~~~~~~~
    std::ostream & operator<< (std::ostream & stream, uuid::version_type version) {
        char const * str = "";
        switch (version) {
        case uuid::version_type::time_based: str = "time_based"; break;
        case uuid::version_type::dce_security: str = "dce_security"; break;
        case uuid::version_type::name_based_md5: str = "name_based_md5"; break;
        case uuid::version_type::random_number_based: str = "random_number_based"; break;
        case uuid::version_type::name_based_sha1: str = "name_based_sha1"; break;
        case uuid::version_type::unknown: str = "unknown"; break;
        };
        return stream << str;
    }

    std::ostream & operator<< (std::ostream & stream, uuid::variant_type variant) {
        char const * str = "";
        switch (variant) {
        case uuid::variant_type::ncs: str = "ncs"; break;
        case uuid::variant_type::rfc_4122: str = "rfc_4122"; break;
        case uuid::variant_type::microsoft: str = "microsoft"; break;
        case uuid::variant_type::future: str = "future"; break; // future definition
        };
        return stream << str;
    }

    std::ostream & operator<< (std::ostream & stream, uuid const & m) { return stream << m.str (); }
} // namespace pstore
