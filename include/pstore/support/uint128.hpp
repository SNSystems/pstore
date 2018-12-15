//*        _       _   _ ____  ___   *
//*  _   _(_)_ __ | |_/ |___ \( _ )  *
//* | | | | | '_ \| __| | __) / _ \  *
//* | |_| | | | | | |_| |/ __/ (_) | *
//*  \__,_|_|_| |_|\__|_|_____\___/  *
//*                                  *
//===- include/pstore/support/uint128.hpp ---------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

#ifndef PSTORE_SUPPORT_UINT128_HPP
#define PSTORE_SUPPORT_UINT128_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include "pstore/config/config.hpp"
#include "pstore/support/portab.hpp"

namespace pstore {
    class uint128;
} // end anonymous namespace

namespace std {
    template <>
    struct is_unsigned<pstore::uint128> : true_type {};
    template <>
    struct is_signed<pstore::uint128> : false_type {};

#if PSTORE_HAVE_UINT128_T && !PSTORE_HAVE_UINT128_TRAITS_SUPPORT
    template <>
    struct is_unsigned<__uint128_t> : true_type {};
    template <>
    struct is_signed<__uint128_t> : false_type {};
#endif // PSTORE_HAVE_UINT128_T && !PSTORE_HAVE_UINT128_TRAITS_SUPPORT

} // namespace std

namespace pstore {

    class alignas (16) uint128 {
    public:
        constexpr uint128 () noexcept = default;
        constexpr uint128 (std::uint64_t high, std::uint64_t low) noexcept;
        constexpr uint128 (std::nullptr_t) noexcept = delete;
#if PSTORE_HAVE_UINT128_T
        /// Construct from an unsigned integer that's 128-bits wide or fewer.
        template <typename IntType,
                  typename = typename std::enable_if<std::is_unsigned<IntType>::value>::type>
        constexpr uint128 (IntType v) noexcept
                : v_{v} {}

        /// \param bytes Points to an array of 16 bytes whose contents represent a 128 bit
        /// value.
        explicit uint128 (std::uint8_t const * bytes) noexcept
                : v_{bytes_to_uint128 (bytes)} {}
#else
        /// Construct from an unsigned integer that's 64-bits wide or fewer.
        template <typename IntType,
                  typename = typename std::enable_if<std::is_unsigned<IntType>::value>::type>
        constexpr uint128 (IntType v) noexcept
                : low_{v} {}

        /// \param bytes Points to an array of 16 bytes whose contents represent a 128 bit
        /// value.
        explicit uint128 (std::uint8_t const * bytes) noexcept
                : low_{bytes_to_uint64 (&bytes[8])}
                , high_{bytes_to_uint64 (&bytes[0])} {}
#endif

        explicit uint128 (std::array<std::uint8_t, 16> const & bytes) noexcept
                : uint128 (bytes.data ()) {}

        constexpr uint128 (uint128 const &) = default;
        constexpr uint128 (uint128 &&) noexcept = default;

        uint128 & operator= (uint128 const &) = default;
        uint128 & operator= (uint128 &&) noexcept = default;

        template <typename T>
        constexpr bool operator== (T rhs) const noexcept;
        template <typename T>
        constexpr bool operator!= (T rhs) const noexcept {
            return !operator== (rhs);
        }

#if PSTORE_HAVE_UINT128_T
        constexpr std::uint64_t high () const noexcept {
            return static_cast<std::uint64_t> (v_ >> 64);
        }
        constexpr std::uint64_t low () const noexcept {
            return static_cast<std::uint64_t> (v_ & max64);
        }
#else
        constexpr std::uint64_t high () const noexcept { return high_; }
        constexpr std::uint64_t low () const noexcept { return low_; }
#endif

        uint128 operator- () const noexcept;
        constexpr bool operator! () const noexcept;
        uint128 operator~ () const noexcept;

        uint128 & operator++ () noexcept;
        uint128 operator++ (int) noexcept;
        uint128 & operator-- () noexcept;
        uint128 operator-- (int) noexcept;

        uint128 & operator+= (uint128 b);
        uint128 & operator-= (uint128 b) { return *this += -b; }

        template <typename T>
        constexpr uint128 operator& (T rhs) const noexcept;
        template <typename T>
        constexpr uint128 operator| (T rhs) const noexcept;

        template <typename T>
        uint128 operator<< (T n) const noexcept;
        uint128 & operator>>= (unsigned n) noexcept;

        std::string to_hex_string () const;

    private:
        static constexpr std::uint64_t max64 = std::numeric_limits<std::uint64_t>::max ();
#if PSTORE_HAVE_UINT128_T
        __uint128_t v_ = 0U;
        static __uint128_t bytes_to_uint128 (std::uint8_t const * bytes) noexcept;
#else
        std::uint64_t low_ = 0U;
        std::uint64_t high_ = 0U;
        static std::uint64_t bytes_to_uint64 (std::uint8_t const * bytes) noexcept;
#endif // PSTORE_HAVE_UINT128_T
    };

    PSTORE_STATIC_ASSERT (sizeof (uint128) == 16);
    PSTORE_STATIC_ASSERT (alignof (uint128) == 16);
    PSTORE_STATIC_ASSERT (std::is_standard_layout<uint128>::value);

    // ctor
    // ~~~~
#if PSTORE_HAVE_UINT128_T
    inline constexpr uint128::uint128 (std::uint64_t high, std::uint64_t low) noexcept
            : v_{__uint128_t{high} << 64 | __uint128_t{low}} {}
#else
    inline constexpr uint128::uint128 (std::uint64_t high, std::uint64_t low) noexcept
            : low_{low}
            , high_{high} {}
#endif

    // operator==
    // ~~~~~~~~~~
    template <typename T>
    constexpr inline bool uint128::operator== (T rhs) const noexcept {
#if PSTORE_HAVE_UINT128_T
        return v_ == rhs;
#else
        return this->high () == 0U && this->low () == rhs;
#endif
    }

    template <>
    constexpr inline bool uint128::operator==<uint128> (uint128 rhs) const noexcept {
#if PSTORE_HAVE_UINT128_T
        return v_ == rhs.v_;
#else
        return this->high () == rhs.high () && this->low () == rhs.low ();
#endif
    }

    // operator++
    // ~~~~~~~~~~
    inline uint128 & uint128::operator++ () noexcept {
#if PSTORE_HAVE_UINT128_T
        ++v_;
#else
        if (++low_ == 0) {
            ++high_;
        }
#endif
        return *this;
    }
    inline uint128 uint128::operator++ (int) noexcept {
        auto const prev = *this;
        ++(*this);
        return prev;
    }

    // operator--
    // ~~~~~~~~~~
    inline uint128 & uint128::operator-- () noexcept {
#if PSTORE_HAVE_UINT128_T
        --v_;
#else
        if (low_-- == 0) {
            --high_;
        }
#endif
        return *this;
    }

    inline uint128 uint128::operator-- (int) noexcept {
        auto const prev = *this;
        --(*this);
        return prev;
    }

    // operator+=
    // ~~~~~~~~~~
    inline uint128 & uint128::operator+= (uint128 rhs) {
#if PSTORE_HAVE_UINT128_T
        v_ += rhs.v_;
#else
        auto const old_low = low_;
        low_ += rhs.low_;
        high_ += rhs.high_;
        if (low_ < old_low) {
            ++high_;
        }
#endif
        return *this;
    }

    // operator-() (unary negation)
    // ~~~~~~~~~~~
    inline uint128 uint128::operator- () const noexcept {
#if PSTORE_HAVE_UINT128_T
        return {-v_};
#else
        auto r = ~(*this);
        return ++r;
#endif
    }

    // operator!
    // ~~~~~~~~~
    inline constexpr bool uint128::operator! () const noexcept {
#if PSTORE_HAVE_UINT128_T
        return !v_;
#else
        return !(high_ != 0 || low_ != 0);
#endif
    }

    // operator~ (binary ones complement)
    // ~~~~~~~~~
    inline uint128 uint128::operator~ () const noexcept {
#if PSTORE_HAVE_UINT128_T
        return {~v_};
#else
        auto t = *this;
        t.low_ = ~t.low_;
        t.high_ = ~t.high_;
        return t;
#endif
    }

    // operator&
    // ~~~~~~~~~
    template <typename T>
    constexpr inline uint128 uint128::operator& (T rhs) const noexcept {
#if PSTORE_HAVE_UINT128_T
        return {v_ & rhs};
#else
        return {0U, low () & rhs};
#endif
    }

    template <>
    constexpr inline uint128 uint128::operator&<uint128> (uint128 rhs) const noexcept {
#if PSTORE_HAVE_UINT128_T
        return {v_ & rhs.v_};
#else
        return {high () & rhs.high (), low () & rhs.low ()};
#endif
    }

    // operator|
    // ~~~~~~~~~
    template <typename T>
    constexpr uint128 uint128::operator| (T rhs) const noexcept {
#if PSTORE_HAVE_UINT128_T
        return {v_ | rhs};
#else
        return {0U, low () | rhs};
#endif
    }

    template <>
    constexpr inline uint128 uint128::operator|<uint128> (uint128 rhs) const noexcept {
#if PSTORE_HAVE_UINT128_T
        return {v_ | rhs.v_};
#else
        return {high () | rhs.high (), low () | rhs.low ()};
#endif
    }

    // operator<<
    // ~~~~~~~~~~
    template <typename Other>
    uint128 uint128::operator<< (Other n) const noexcept {
        assert (n <= 128);
#if PSTORE_HAVE_UINT128_T
        return {v_ << n};
#else
        if (n >= 64U) {
            return uint128{low () << (n - 64U), 0U};
        }
        std::uint64_t const mask = (std::uint64_t{1} << n) - 1U;
        std::uint64_t const top_of_low_mask = mask << (64U - n);
        std::uint64_t const bottom_of_high = (low () & top_of_low_mask) >> (64U - n);
        return {(high () << n) | bottom_of_high, low () << n};
#endif
    }

    // operator>>=
    // ~~~~~~~~~~~
    inline uint128 & uint128::operator>>= (unsigned n) noexcept {
        assert (n <= 128);
#if PSTORE_HAVE_UINT128_T
        v_ >>= n;
#else
        if (n >= 64) {
            low_ = high_ >> (n - 64U);
            high_ = 0U;
        } else {
            std::uint64_t const mask = (std::uint64_t{1} << n) - 1U;
            low_ = (low_ >> n) | ((high_ & mask) << (64U - n));
            high_ >>= n;
        }
#endif
        return *this;
    }

    // bytes_to_uintXX
    // ~~~~~~~~~~~~~~~
#if PSTORE_HAVE_UINT128_T
    inline __uint128_t uint128::bytes_to_uint128 (std::uint8_t const * bytes) noexcept {
        return __uint128_t{bytes[0]} << 120 | __uint128_t{bytes[1]} << 112 |
               __uint128_t{bytes[2]} << 104 | __uint128_t{bytes[3]} << 96 |
               __uint128_t{bytes[4]} << 88 | __uint128_t{bytes[5]} << 80 |
               __uint128_t{bytes[6]} << 72 | __uint128_t{bytes[7]} << 64 |
               __uint128_t{bytes[8]} << 56 | __uint128_t{bytes[9]} << 48 |
               __uint128_t{bytes[10]} << 40 | __uint128_t{bytes[11]} << 32 |
               __uint128_t{bytes[12]} << 24 | __uint128_t{bytes[13]} << 16 |
               __uint128_t{bytes[14]} << 8 | __uint128_t{bytes[15]};
    }
#else
    inline std::uint64_t uint128::bytes_to_uint64 (std::uint8_t const * bytes) noexcept {
        return std::uint64_t{bytes[0]} << 56 | std::uint64_t{bytes[1]} << 48 |
               std::uint64_t{bytes[2]} << 40 | std::uint64_t{bytes[3]} << 32 |
               std::uint64_t{bytes[4]} << 24 | std::uint64_t{bytes[5]} << 16 |
               std::uint64_t{bytes[6]} << 8 | std::uint64_t{bytes[7]};
    }
#endif

    inline std::ostream & operator<< (std::ostream & os, uint128 const & value) {
        return os << '{' << value.high () << ',' << value.low () << '}';
    }

    inline constexpr bool operator== (uint128 const & lhs, uint128 const & rhs) noexcept {
        return lhs.operator== (rhs);
    }
    inline constexpr bool operator!= (uint128 const & lhs, uint128 const & rhs) noexcept {
        return lhs.operator!= (rhs);
    }

    inline constexpr bool operator< (uint128 const & lhs, uint128 const & rhs) noexcept {
        return lhs.high () < rhs.high () ||
               (!(rhs.high () < lhs.high ()) && lhs.low () < rhs.low ());
    }
    inline constexpr bool operator> (uint128 const & lhs, uint128 const & rhs) noexcept {
        return rhs < lhs;
    }
    inline constexpr bool operator>= (uint128 const & lhs, uint128 const & rhs) noexcept {
        return !(lhs < rhs);
    }
    inline constexpr bool operator<= (uint128 const & lhs, uint128 const & rhs) noexcept {
        return !(rhs < lhs);
    }

} // end namespace pstore

namespace std {
    template <>
    struct hash<pstore::uint128> {
        using argument_type = pstore::uint128;
        using result_type = size_t;

        result_type operator() (argument_type const & s) const {
            return std::hash<std::uint64_t>{}(s.low ()) ^ std::hash<std::uint64_t>{}(s.high ());
        }
    };

    // TODO: strictly speaking we should be providing const and volatile versions of this
    // specialization as well.
    template <>
    class numeric_limits<pstore::uint128> {
        using type = pstore::uint128;
    public:
        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = false;

        static constexpr type min () noexcept { return {0U}; }
        static constexpr type max () noexcept {
            return {std::numeric_limits<std::uint64_t>::max (),
                    std::numeric_limits<std::uint64_t>::max ()};
        }
        static constexpr type lowest () noexcept { return type{0U}; }

        static constexpr const int digits = 128;
        static constexpr const int digits10 = 38;
        static constexpr const int max_digits10 = 0;
        static constexpr const bool is_integer = true;
        static constexpr const bool is_exact = true;
        static constexpr const int radix = 2;
        static constexpr type epsilon () noexcept { return {0U}; }
        static constexpr type round_error () noexcept { return {0U}; }

        static constexpr const int min_exponent = 0;
        static constexpr const int min_exponent10 = 0;
        static constexpr const int max_exponent = 0;
        static constexpr const int max_exponent10 = 0;

        static constexpr const bool has_infinity = false;
        static constexpr const bool has_quiet_NaN = false;
        static constexpr const bool has_signaling_NaN = false;
        static constexpr const float_denorm_style has_denorm = denorm_absent;
        static constexpr const bool has_denorm_loss = false;
        static constexpr type infinity () noexcept { return {0U}; }
        static constexpr type quiet_NaN () noexcept { return {0U}; }
        static constexpr type signaling_NaN () noexcept { return {0U}; }
        static constexpr type denorm_min () noexcept { return {0U}; }

        static constexpr const bool is_iec559 = false;
        static constexpr const bool is_bounded = true;
        static constexpr const bool is_modulo = true;

        static constexpr const bool traps = true;
        static constexpr const bool tinyness_before = false;
        static constexpr const float_round_style round_style = std::round_toward_zero;
    };

} // end namespace std

#endif // PSTORE_SUPPORT_UINT128_HPP
