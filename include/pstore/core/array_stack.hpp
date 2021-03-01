//===- include/pstore/core/array_stack.hpp ----------------*- mode: C++ -*-===//
//*                                   _             _     *
//*   __ _ _ __ _ __ __ _ _   _   ___| |_ __ _  ___| | __ *
//*  / _` | '__| '__/ _` | | | | / __| __/ _` |/ __| |/ / *
//* | (_| | |  | | | (_| | |_| | \__ \ || (_| | (__|   <  *
//*  \__,_|_|  |_|  \__,_|\__, | |___/\__\__,_|\___|_|\_\ *
//*                       |___/                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file array_stack.hpp
/// \brief array_stack is a simple container which provides an interface very
/// similar to std::stack, but based on std::array.

#ifndef PSTORE_CORE_ARRAY_STACK_HPP
#define PSTORE_CORE_ARRAY_STACK_HPP

#include <array>
#include <cstdlib>

#include "pstore/support/assert.hpp"

namespace pstore {

    /// A simple wrapper for std::array which provides the functionality of a stack,
    /// specifically a FILO (first-in, last-out) data structure. Its interface is
    /// close to that of std::stack<>, but an additional class is necessary because
    /// std::array<> does not fully meet the requirements of std::stack<>.
    ///
    /// Use this class if the stack size is known a priori to be small and of known
    /// maximum depth.

    template <class Ty, std::size_t Size>
    class array_stack {
    public:
        using container_type = std::array<Ty, Size>;
        using value_type = typename container_type::value_type;
        using reference = typename container_type::reference;
        using const_reference = typename container_type::const_reference;
        using size_type = typename container_type::size_type;

        array_stack () = default;

        bool operator== (array_stack const & other) const {
            return elements_ == other.elements_ && std::equal (begin (), end (), other.begin ());
        }
        bool operator!= (array_stack const & other) const { return !operator== (other); }

        /// Returns an iterator pointing to the first element in the stack.
        typename container_type::const_iterator begin () const { return std::begin (c_); }
        /// Returns an iterator pointing to the past-the-end element in the array container.
        typename container_type::const_iterator end () const { return std::begin (c_) + elements_; }

        /// \name Capacity
        ///@{

        /// Checks whether the stack is empty.
        constexpr bool empty () const noexcept { return elements_ == 0; }

        /// Returns the number of elements stored on the stack.
        constexpr size_type size () const noexcept { return elements_; }

        /// Returns the maximum number of elements that the stack is able to
        /// hold.
        constexpr size_t max_size () const noexcept { return Size; }
        ///@}


        /// \name Element access
        ///@{

        /// Acceses the top element
        reference top () noexcept {
            PSTORE_ASSERT (elements_ > 0);
            return c_[elements_ - 1];
        }
        /// Acceses the top element
        const_reference top () const noexcept {
            PSTORE_ASSERT (elements_ > 0);
            return c_[elements_ - 1];
        }

        /// \name Modifiers
        ///@{

        /// Inserts an element at the top of the container.
        /// \param value The value of the element to push
        void push (value_type const & value) {
            PSTORE_ASSERT (elements_ < Size);
            c_[elements_++] = value;
        }
        /// Inserts an element at the top of the container.
        /// \param value The value of the element to push
        void push (value_type && value) {
            PSTORE_ASSERT (elements_ < Size);
            c_[elements_++] = std::move (value);
        }

        /// Remove the top element from the stack.
        void pop () {
            PSTORE_ASSERT (elements_ > 0);
            --elements_;
        }
        ///@}

    private:
        /// The array which holds the stack contents.
        container_type c_;
        /// The number of elements on the stack. Always >= 0 and < Size.
        std::size_t elements_{0};
    };
} // namespace pstore

#endif // PSTORE_CORE_ARRAY_STACK_HPP
