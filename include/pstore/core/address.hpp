//*            _     _                    *
//*   __ _  __| | __| |_ __ ___  ___ ___  *
//*  / _` |/ _` |/ _` | '__/ _ \/ __/ __| *
//* | (_| | (_| | (_| | | |  __/\__ \__ \ *
//*  \__,_|\__,_|\__,_|_|  \___||___/___/ *
//*                                       *
//===- include/pstore/core/address.hpp ------------------------------------===//
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
/// \file address.hpp

//  --> increasing addresses
//
//   |<- j ->|<- j ->|
// |H| s(0)  |  s(1) |  s(2) | s(3)  | s(4) |T|
//   |-------+-------+-------+-------+------+
//   | region                | region       |
//   |       0               |       1      |


//                    existing segments <---|---> new segments
//   |<- j ->|<- j ->|
// |H| s(0)  |  s(1) |  s(2) | s(3)  | s(4) | s(n-1) |  s(n)   |T|
//   |-------+-------+-------+-------+------+--------+---------+
//   | region                | region       |region  | region  |
//   |       0               |       1      |      2 |       3 |


// |<- j ->|<- j ->|
// | s(0)  | s(1)  | s(2)  | s(3)  | s(4) | s(n-1) | s(n) |
// |-----------------------+--------------+---------------+
// | region                | region       |
// |       0               |       1      |
// |-----------------------+--------------+---------------+
// | m                     | m            | m     | m     |
// |  0                    |  1           |  2    |  3    |
//

// H is the file header
// j is 2^offset_number_bits
// s is a segment number
// m is an entry in the mapping table



// Regions are used to limit the amount of contiguous address space that we request from the OS.

// When I open the file, I divide the space into regions. On Windows, each region corresponds to
// a "file mapping". is memory mapped and then the underlying segment pointers are pushed onto
// the segment address table. Any remaining segments are then mapped and added to the table.
// (There isn't a one-to-one correspondence between regions or segments and "file mappings" (on
// Windows). file is modified we decide if a new segment needs to be created.

#ifndef PSTORE_ADDRESS_HPP
#define PSTORE_ADDRESS_HPP

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iosfwd>
#include <limits>
#include <type_traits>

#include "pstore/support/portab.hpp"

namespace pstore {

#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304
#define PSTORE_CONST_EXPR_ASSERT(cond) assert (cond)
#else
#define PSTORE_CONST_EXPR_ASSERT(cond)
#endif

    //*            _     _                    *
    //*   __ _  __| | __| |_ __ ___  ___ ___  *
    //*  / _` |/ _` |/ _` | '__/ _ \/ __/ __| *
    //* | (_| | (_| | (_| | | |  __/\__ \__ \ *
    //*  \__,_|\__,_|\__,_|_|  \___||___/___/ *
    //*                                       *
    struct address {
        /// An offset is 0-2^22 (4 megabytes)
        static constexpr unsigned const offset_number_bits = 22U;
        /// A segment number is 0-2^16
        static constexpr unsigned const segment_number_bits = 16U;

        using offset_type = std::uint_least32_t;
        using segment_type = std::uint_least16_t;

        static constexpr std::uint64_t as_absolute (segment_type segment,
                                                    offset_type offset) noexcept {
            PSTORE_CONST_EXPR_ASSERT (std::uint64_t{segment} <= max_segment + UINT64_C (1));
            PSTORE_CONST_EXPR_ASSERT (std::uint64_t{offset} <= max_offset + UINT64_C (1));
            return (std::uint64_t{segment} << offset_number_bits) | std::uint64_t{offset};
        }

        /// The largest legal offset value.
        static constexpr offset_type const max_offset =
            (UINT64_C (1) << offset_number_bits) - UINT64_C (1);
        /// The largest legal segment value
        static constexpr segment_type const max_segment = (1U << segment_number_bits) - 1U;
        /// The largest legal absolute address.
        static constexpr std::uint64_t const max_address =
            (std::uint64_t{max_segment} << offset_number_bits) | std::uint64_t{max_offset};

        /// The number of bytes in a segment.
        static constexpr std::uint64_t const segment_size =
            std::uint64_t{max_offset} + UINT64_C (1);

        static constexpr address make (std::uint64_t absolute) noexcept { return {absolute}; }
        static constexpr address make (segment_type segment, offset_type offset) noexcept {
            return {as_absolute (segment, offset)};
        }
        static constexpr address null () noexcept { return {0}; }
        static constexpr address max () noexcept { return {max_address}; }


        constexpr std::uint64_t absolute () const noexcept { return whole; }

        address operator++ () noexcept { return (*this) += 1U; }
        address operator++ (int) noexcept {
            auto const old = *this;
            ++(*this);
            return old;
        }
        address operator-- () noexcept { return (*this) -= 1U; }
        address operator-- (int) noexcept {
            auto const old = *this;
            --(*this);
            return old;
        }

        address operator+= (std::uint64_t distance) noexcept {
            assert (whole <= std::numeric_limits<std::uint64_t>::max () - distance);
            whole += distance;
            return *this;
        }

        address operator-= (std::uint64_t distance) noexcept {
            assert (whole >= distance);
            whole -= distance;
            return *this;
        }

        address operator|= (std::uint64_t mask) noexcept {
            whole |= mask;
            return *this;
        }

        address operator&= (std::uint64_t mask) noexcept {
            whole &= mask;
            return *this;
        }

        segment_type segment () const noexcept {
            constexpr auto const segment_mask = std::uint64_t{max_segment} << offset_number_bits;
            return static_cast<segment_type> ((whole & segment_mask) >> offset_number_bits);
        }

        offset_type offset () const noexcept {
            return static_cast<offset_type> (whole & max_offset);
        }

        std::uint64_t whole;
    };

    static_assert (sizeof (address) == 8, "address should be 8 bytes wide");
    static_assert (std::is_standard_layout<address>::value, "address is not standard layout");

    // comparison

    inline bool operator== (address const & lhs, address const & rhs) noexcept {
        return lhs.absolute () == rhs.absolute ();
    }
    inline bool operator!= (address const & lhs, address const & rhs) noexcept {
        return !operator== (lhs, rhs);
    }


    // ordering

    inline bool operator> (address const & lhs, address const & rhs) noexcept {
        return lhs.absolute () > rhs.absolute ();
    }
    inline bool operator>= (address const & lhs, address const & rhs) noexcept {
        return lhs.absolute () >= rhs.absolute ();
    }
    inline bool operator< (address const & lhs, address const & rhs) noexcept {
        return lhs.absolute () < rhs.absolute ();
    }
    inline bool operator<= (address const & lhs, address const & rhs) noexcept {
        return lhs.absolute () <= rhs.absolute ();
    }


    // arithmetic

    inline address operator- (address const lhs, std::uint64_t rhs) noexcept {
        assert (lhs.absolute () >= rhs);
        return address::make (lhs.absolute () - rhs);
    }
    inline address operator- (address const lhs, address const rhs) noexcept {
        assert (lhs.absolute () >= rhs.absolute ());
        return address::make (lhs.absolute () - rhs.absolute ());
    }

    inline address operator+ (address const lhs, std::uint64_t rhs) noexcept {
        return address::make (lhs.absolute () + rhs);
    }
    inline address operator+ (address const lhs, address rhs) noexcept {
        return address::make (lhs.absolute () + rhs.absolute ());
    }


    // bitwise

    inline address operator| (address const lhs, std::uint64_t rhs) noexcept {
        return address::make (lhs.absolute () | rhs);
    }

    std::ostream & operator<< (std::ostream & os, address const & addr);
} // end namespace pstore


namespace pstore {

    template <typename T>
    class typed_address {
    public:
        using type = T;

        typed_address () = default;
        explicit constexpr typed_address (address a) noexcept
                : a_{a} {}
        typed_address (typed_address const & addr) noexcept = default;

        template <typename Other>
        static constexpr typed_address cast (typed_address<Other> other) noexcept {
            return make (other.to_address ());
        }

        static constexpr typed_address null () noexcept { return make (address::null ()); }
        static constexpr typed_address make (address a) noexcept { return typed_address (a); }
        static constexpr typed_address make (std::uint64_t absolute) noexcept {
            return typed_address (address::make (absolute));
        }

        typed_address & operator= (typed_address const & rhs) noexcept = default;
        constexpr bool operator== (typed_address const & rhs) const noexcept {
            return a_ == rhs.a_;
        }
        constexpr bool operator!= (typed_address const & rhs) const noexcept {
            return !operator== (rhs);
        }

        typed_address operator++ () noexcept { return (*this) += 1U; }
        typed_address operator++ (int) noexcept {
            auto const old = *this;
            ++(*this);
            return old;
        }
        typed_address operator-- () noexcept { return (*this) -= 1U; }
        typed_address operator-- (int) noexcept {
            auto const old = *this;
            --(*this);
            return old;
        }

        typed_address operator+= (std::uint64_t distance) noexcept {
            a_ += distance * sizeof (T);
            return *this;
        }
        typed_address operator-= (std::uint64_t distance) noexcept {
            a_ -= distance * sizeof (T);
            return *this;
        }

        constexpr address to_address () const noexcept { return a_; }
        constexpr std::uint64_t absolute () const noexcept { return a_.absolute (); }

    private:
        address a_;
    };
    // ordering

    template <typename T>
    inline bool operator> (typed_address<T> lhs, typed_address<T> rhs) noexcept {
        return lhs.to_address () > rhs.to_address ();
    }
    template <typename T>
    inline bool operator>= (typed_address<T> lhs, typed_address<T> rhs) noexcept {
        return lhs.to_address () >= rhs.to_address ();
    }
    template <typename T>
    inline bool operator< (typed_address<T> lhs, typed_address<T> rhs) noexcept {
        return lhs.to_address () < rhs.to_address ();
    }
    template <typename T>
    inline bool operator<= (typed_address<T> lhs, typed_address<T> rhs) noexcept {
        return lhs.to_address () <= rhs.to_address ();
    }

    // arithmetic

    template <typename T>
    inline typed_address<T> operator- (typed_address<T> const lhs, std::uint64_t rhs) noexcept {
        auto const delta = rhs * sizeof (T);
        assert (lhs.absolute () >= delta);
        return typed_address<T> (lhs.to_address () - delta);
    }

    template <typename T>
    inline typed_address<T> operator+ (typed_address<T> const lhs, std::uint64_t rhs) noexcept {
        return typed_address<T> (address::make (lhs.absolute () + rhs * sizeof (T)));
    }

    template <typename T>
    std::ostream & operator<< (std::ostream & os, typed_address<T> const & addr) {
        return os << addr.to_address ();
    }

} // end namespace pstore


namespace std {
    //*                                  _        _ _           _ _        *
    //*  _ __  _   _ _ __ ___   ___ _ __(_) ___  | (_)_ __ ___ (_) |_ ___  *
    //* | '_ \| | | | '_ ` _ \ / _ \ '__| |/ __| | | | '_ ` _ \| | __/ __| *
    //* | | | | |_| | | | | | |  __/ |  | | (__  | | | | | | | | | |_\__ \ *
    //* |_| |_|\__,_|_| |_| |_|\___|_|  |_|\___| |_|_|_| |_| |_|_|\__|___/ *
    //*                                                                    *

    // TODO: strictly speaking we should be providing const and volatile versions of this
    // specialization as well.
    template <>
    class numeric_limits<pstore::address> {
        using base = std::numeric_limits<std::uint64_t>;
        using type = pstore::address;

    public:
        static constexpr const bool is_specialized = base::is_specialized;
        static constexpr type min () noexcept { return pstore::address::null (); }
        static constexpr type max () noexcept { return pstore::address::max (); }
        static constexpr type lowest () noexcept { return pstore::address::make (base::lowest ()); }

        static constexpr const int digits = base::digits;
        static constexpr const int digits10 = base::digits10;
        static constexpr const int max_digits10 = base::max_digits10;
        static constexpr const bool is_signed = base::is_signed;
        static constexpr const bool is_integer = base::is_integer;
        static constexpr const bool is_exact = base::is_exact;
        static constexpr const int radix = base::radix;
        static constexpr type epsilon () noexcept {
            return pstore::address::make (base::epsilon ());
        }
        static constexpr type round_error () noexcept {
            return pstore::address::make (base::round_error ());
        }

        static constexpr const int min_exponent = base::min_exponent;
        static constexpr const int min_exponent10 = base::min_exponent10;
        static constexpr const int max_exponent = base::max_exponent;
        static constexpr const int max_exponent10 = base::max_exponent10;

        static constexpr const bool has_infinity = base::has_infinity;
        static constexpr const bool has_quiet_NaN = base::has_quiet_NaN;
        static constexpr const bool has_signaling_NaN = base::has_signaling_NaN;
        static constexpr const float_denorm_style has_denorm = base::has_denorm;
        static constexpr const bool has_denorm_loss = base::has_denorm_loss;
        static constexpr type infinity () noexcept {
            return pstore::address::make (base::infinity ());
        }
        static constexpr type quiet_NaN () noexcept {
            return pstore::address::make (base::quiet_NaN ());
        }
        static constexpr type signaling_NaN () noexcept {
            return pstore::address::make (base::signaling_NaN ());
        }
        static constexpr type denorm_min () noexcept {
            return pstore::address::make (base::denorm_min ());
        }

        static constexpr const bool is_iec559 = base::is_iec559;
        static constexpr const bool is_bounded = base::is_bounded;
        static constexpr const bool is_modulo = base::is_modulo;

        static constexpr const bool traps = base::traps;
        static constexpr const bool tinyness_before = base::tinyness_before;
        static constexpr const float_round_style round_style = base::round_style;
    };

    //*  _                  _      *
    //* | |__    __ _  ___ | |__   *
    //* | '_ \  / _` |/ __|| '_ \  *
    //* | | | || (_| |\__ \| | | | *
    //* |_| |_| \__,_||___/|_| |_| *
    //*                            *
    template <>
    struct hash<pstore::address> {
        using argument_type = pstore::address;
        using result_type = std::size_t;

        result_type operator() (argument_type s) const {
            auto abs = s.absolute ();
            return std::hash<decltype (abs)>{}(abs);
        }
    };

    template <typename T>
    struct hash<pstore::typed_address<T>> {
        using argument_type = pstore::typed_address<T>;
        using result_type = std::size_t;

        result_type operator() (argument_type s) const {
            auto addr = s.to_address ();
            return std::hash<decltype (addr)>{}(addr);
        }
    };

} // namespace std

#endif // PSTORE_ADDRESS_HPP
