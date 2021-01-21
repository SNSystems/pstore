//*  _     _ _      __ _      _     _  *
//* | |__ (_) |_   / _(_) ___| | __| | *
//* | '_ \| | __| | |_| |/ _ \ |/ _` | *
//* | |_) | | |_  |  _| |  __/ | (_| | *
//* |_.__/|_|\__| |_| |_|\___|_|\__,_| *
//*                                    *
//===- include/pstore/support/bit_field.hpp -------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file bit_field.hpp
/// \brief A portable bit-field type.
///
/// C and C++ bit-fields are a nice easy way to declare structure members with an explicit number of
/// bits. However, their portability is severely limited if the exact layout of the structure is
/// important: the language standards allow the compiler complete freedom in deciding where are how
/// the bits are allocated in the underlying memory. We need some of the pstore data structures to
/// be consistent regardless of the compiler being used, and there are static assertions which
/// enforce these guarantees. Unfortunately, if we can't guarantee the bit-field layout then we
/// can't used traditional bit-fields in these structures.
///
/// The template class provded here is a simple means to define a structure member which uses
/// specified bits from a value of a specified base type.
///
/// Using it is simple. Define a union consisting of a member of the base type and one member for
/// each of the bit-fields. For example:
///
///     union {
///         std::uint8_t v;
///         bit_field<std::uint8_t, 0, 2> f1; // f1 is bits [0-2)
///         bit_field<std::uint8_t, 2, 6> f2; // f2 is bits [2-7)
///     };
///
/// This is approximately equivalent to the standard C:
///
///     struct {
///         std::uint8_t f1 : 2;
///         std::uint8_t f2 : 6;
///     };
///
/// The difference is that in the first example the union will occupy 8 bits with f1 being bits
/// [0-2) and f1 beng bits [2-7). The standards make few guarantees about the second example.
///
/// It's worth noting that the 'v' member of the first example should be initialized to 0 _before_
/// fields f1 or f2 are read or written. This is to ensure the masks performed by the bit-field
/// instances don't try to read or write uninitialized memory.

#ifndef PSTORE_SUPPORT_BIT_FIELD_HPP
#define PSTORE_SUPPORT_BIT_FIELD_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "pstore/support/assert.hpp"

namespace pstore {

    namespace details {

        template <typename ValueType, unsigned Bits>
        struct max_value {
            static constexpr ValueType value = static_cast<ValueType> ((ValueType{1} << Bits) - 1);
        };

        template <typename ValueType>
        struct max_value<ValueType, 8U> {
            static constexpr ValueType value = std::numeric_limits<std::uint8_t>::max ();
        };

        template <typename ValueType>
        struct max_value<ValueType, 16U> {
            static constexpr ValueType value = std::numeric_limits<std::uint16_t>::max ();
        };

        template <typename ValueType>
        struct max_value<ValueType, 32U> {
            static constexpr ValueType value = std::numeric_limits<std::uint32_t>::max ();
        };

        template <typename ValueType>
        struct max_value<ValueType, 64U> {
            static constexpr ValueType value = std::numeric_limits<std::uint64_t>::max ();
        };

        template <typename ValueType, unsigned Bits>
        constexpr ValueType max_value<ValueType, Bits>::value;
        template <typename ValueType>
        constexpr ValueType max_value<ValueType, 8U>::value;
        template <typename ValueType>
        constexpr ValueType max_value<ValueType, 16U>::value;
        template <typename ValueType>
        constexpr ValueType max_value<ValueType, 32U>::value;
        template <typename ValueType>
        constexpr ValueType max_value<ValueType, 64U>::value;


        template <typename ValueType, unsigned Index, unsigned Bits,
                  typename = typename std::enable_if<std::is_unsigned<ValueType>::value &&
                                                     Index + Bits <= sizeof (ValueType) * 8>::type>
        class bit_field_base {
        public:
            static constexpr auto first_bit = Index;
            static constexpr auto last_bit = Index + Bits;
            /// The underlying type used to store this bit-field.
            using value_type = ValueType;

            /// Returns the smallest value that can be stored in this bit-field.
            static constexpr value_type min () noexcept { return 0; }
            /// Returns the largest value that can be stored in this bit-field.
            static constexpr value_type max () noexcept { return mask_; }

            /// Returns the value stored in this bit-field.
            constexpr value_type value () const noexcept {
                return (this->value_ >> Index) & this->mask_;
            }

            template <typename T,
                      typename = typename std::enable_if<std::is_unsigned<T>::value &&
                                                         sizeof (T) <= sizeof (value_type)>::type>
            void assign (T v) noexcept {
                value_ = static_cast<value_type> (value_ & ~(mask_ << Index)) |
                         static_cast<value_type> ((v & mask_) << Index);
            }

        private:
            static constexpr auto mask_ = max_value<ValueType, Bits>::value;
            value_type value_;
        };

    } // end namespace details


    template <typename ValueType, unsigned Index, unsigned Bits>
    class bit_field : private details::bit_field_base<ValueType, Index, Bits> {
        using inherited = details::bit_field_base<ValueType, Index, Bits>;

    public:
        using inherited::first_bit;
        using inherited::last_bit;
        using inherited::max;
        using inherited::min;
        /// The underlying type used to store this bit-field.
        using typename inherited::value_type;
        /// The canonical type that can be assigned to a bit-field instance.
        using assign_type = ValueType;

        /// obtains the value held in the bit-field.
        using inherited::value;
        /// obtains the value held in the bit-field.
        constexpr operator value_type () const noexcept { return this->value (); }

        /// assigns a value to the bit-field
        template <typename T>
        bit_field & operator= (T v) noexcept {
            this->assign (v);
            return *this;
        }


        template <typename T>
        bit_field & operator+= (T other) noexcept {
            this->assign (static_cast<value_type> (this->value () + other));
            return *this;
        }
        template <typename T>
        bit_field & operator-= (T other) noexcept {
            this->assign (static_cast<value_type> (this->value () - other));
            return *this;
        }

        bit_field & operator++ () noexcept { return operator+=(value_type{1}); }
        bit_field const operator++ (int) noexcept {
            bit_field const prev = *this;
            ++*this;
            return prev;
        }

        bit_field & operator-- () noexcept { return operator-=(value_type{1}); }
        bit_field const operator-- (int) noexcept {
            bit_field const prev = *this;
            --*this;
            return prev;
        }
    };

    template <typename ValueType, unsigned Index>
    class bit_field<ValueType, Index, 1> : private details::bit_field_base<ValueType, Index, 1> {
        using inherited = details::bit_field_base<ValueType, Index, 1>;

    public:
        using inherited::first_bit;
        using inherited::last_bit;
        using inherited::max;
        using inherited::min;
        /// The underlying type used to store this bit-field.
        using typename inherited::value_type;
        /// The canonical type that can be assigned to a bit-field instance.
        using assign_type = bool;

        /// assigns a value to the bit-field
        bit_field & operator= (bool const v) noexcept {
            this->assign (static_cast<value_type> (v));
            return *this;
        }

        constexpr bool value () const noexcept { return static_cast<bool> (inherited::value ()); }
        constexpr operator bool () const noexcept { return value (); }
    };

} // end namespace pstore

#endif // PSTORE_SUPPORT_BIT_FIELD_HPP
