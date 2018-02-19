//*                                   _             _     *
//*   __ _ _ __ _ __ __ _ _   _   ___| |_ __ _  ___| | __ *
//*  / _` | '__| '__/ _` | | | | / __| __/ _` |/ __| |/ / *
//* | (_| | |  | | | (_| | |_| | \__ \ || (_| | (__|   <  *
//*  \__,_|_|  |_|  \__,_|\__, | |___/\__\__,_|\___|_|\_\ *
//*                       |___/                           *
//===- include/pstore/array_stack.hpp -------------------------------------===//
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
/// \file array_stack.hpp
/// \brief array_stack is a simple container which provides an interface very
/// similar to std::stack, but based on std::array.

#ifndef PSTORE_ARRAY_STACK_HPP
#define PSTORE_ARRAY_STACK_HPP

#include <array>
#include <cassert>
#include <cstdlib>

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

    private:
        /// The array which holds the stack contents.
        container_type c_;
        /// The number of elements on the stack. Always >= 0 and < Size.
        std::size_t elements_{0};

    public:
        array_stack () = default;

        /// Returns an iterator pointing to the past-the-end element in the array container.
        bool operator== (array_stack const & other) const {
            return elements_ == other.elements_ && std::equal (begin (), end (), other.begin ());
        }

        /// Returns an iterator pointing to the first element in the stack.
        typename container_type::const_iterator begin () const { return std::begin (c_); }

        typename container_type::const_iterator end () const { return std::begin (c_) + elements_; }

        bool operator!= (array_stack const & other) const { return !operator== (other); }

        /// \name Capacity
        ///@{

        /// Checks whether the stack is empty.
        bool empty () const { return elements_ == 0; }

        /// Returns the number of elements stored on the stack.
        size_type size () const { return elements_; }

        /// Returns the maximum number of elements that the stack is able to
        /// hold.
        constexpr size_t max_size () const { return Size; }
        ///@}


        /// \name Element access
        ///@{

        /// Acceses the top element
        reference top () {
            assert (elements_ > 0);
            return c_[elements_ - 1];
        }
        /// Acceses the top element
        const_reference top () const {
            assert (elements_ > 0);
            return c_[elements_ - 1];
        }

        /// \name Modifiers
        ///@{

        /// Inserts an element at the top of the container.
        /// \param value The value of the element to push
        void push (value_type const & value) {
            assert (elements_ < Size);
            c_[elements_++] = value;
        }
        /// Inserts an element at the top of the container.
        /// \param value The value of the element to push
        void push (value_type && value) {
            assert (elements_ < Size);
            c_[elements_++] = std::move (value);
        }

        /// Remove the top element from the stack.
        void pop () {
            assert (elements_ > 0);
            --elements_;
        }
        ///@}
    };
} // namespace pstore

#endif // PSTORE_ARRAY_STACK_HPP
// eof: include/pstore/array_stack.hpp
