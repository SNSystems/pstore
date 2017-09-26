//*                      _ _                  _              *
//*  ___ _ __ ___   __ _| | | __   _____  ___| |_ ___  _ __  *
//* / __| '_ ` _ \ / _` | | | \ \ / / _ \/ __| __/ _ \| '__| *
//* \__ \ | | | | | (_| | | |  \ V /  __/ (__| || (_) | |    *
//* |___/_| |_| |_|\__,_|_|_|   \_/ \___|\___|\__\___/|_|    *
//*                                                          *
//===- include/pstore_support/small_vector.hpp ----------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc. 
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
/// \file small_vector.hpp
/// \brief Provides a small, normally stack allocated, buffer but which can be
/// resized dynamically when necessary.

#ifndef PSTORE_SMALL_VECTOR_HPP
#define PSTORE_SMALL_VECTOR_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <vector>

namespace pstore {

    /// A class which provides a vector-like interface to a small, normally stack
    /// allocated, buffer which may, if necessary, be resized. It is normally used
    /// to contain string buffers where they are typically small enough to be
    /// stack-allocated, but where the code must gracefully suport arbitrary lengths.
    template <typename ElementType, std::size_t BodyElements = 256>
    class small_vector {
    public:
        typedef ElementType value_type;

        typedef value_type & reference;
        typedef value_type const & const_reference;
        typedef value_type * pointer;
        typedef value_type const * const_pointer;

        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;

        typedef value_type * iterator;
        typedef value_type const * const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;


        /// Constructs the buffer with an initial size of 0.
        small_vector ();
        /// Constructs the buffer with the given initial numbe of elements.
        explicit small_vector (std::size_t required_elements);

        // Move is supported.
        small_vector (small_vector && other);
        small_vector & operator= (small_vector && other);

        // No copying or assignment. They could be implemented but probably
        // wouldn't be terribly efficient.
        small_vector (small_vector const & rhs);
        small_vector & operator= (small_vector const &) = delete;

        /// \name Element access
        ///@{
        ElementType const * data () const {
            return buffer_;
        }
        ElementType * data () {
            return buffer_;
        }

        ElementType const & operator[] (std::size_t index) const {
            assert (index < elements_);
            return buffer_[index];
        }
        ElementType & operator[] (std::size_t index) {
            assert (index < elements_);
            return buffer_[index];
        }
        ///@}

        /// \name Capacity
        ///@{
        /// Returns the number of elements.
        std::size_t size () const {
            return elements_;
        }
        std::size_t size_bytes () const {
            return elements_ * sizeof (ElementType);
        }

        /// Checks whether the container is empty.
        bool empty () const {
            return elements_ == 0;
        }

        /// Returns the number of elements that can be held in currently allocated
        /// storage.
        std::size_t capacity () const {
            return is_small (elements_) ? BodyElements : big_buffer_.capacity ();
        }

        /// Resizes the container so that it is large enough for accommodate the
        /// given number of elements.
        ///
        /// \note Calling this function invalidates the contents of the buffer and
        /// any iterators.
        ///
        /// \param new_elements The number of elements that the buffer is to
        ///                     accommodate.
        void resize (std::size_t new_elements);
        ///@}

        /// \name Iterators
        ///@{
        /// Returns an iterator to the beginning of the container.
        const_iterator begin () const noexcept {
            return buffer_;
        }
        iterator begin () noexcept {
            return buffer_;
        }
        const_iterator cbegin () noexcept {
            return buffer_;
        }
        /// Returns a reverse iterator to the first element of the reversed
        /// container. It corresponds to the last element of the non-reversed
        /// container.
        reverse_iterator rbegin () noexcept {
            return reverse_iterator{this->end ()};
        }
        const_reverse_iterator rbegin () const noexcept {
            return const_reverse_iterator{this->end ()};
        }
        const_reverse_iterator rcbegin () noexcept {
            return const_reverse_iterator{this->end ()};
        }

        /// Returns an iterator to the end of the container.
        const_iterator end () const noexcept {
            return buffer_ + elements_;
        }
        iterator end () noexcept {
            return buffer_ + elements_;
        }
        const_iterator cend () noexcept {
            return buffer_ + elements_;
        }
        reverse_iterator rend () noexcept {
            return reverse_iterator{this->begin ()};
        }
        const_reverse_iterator rend () const noexcept {
            return const_reverse_iterator{this->begin ()};
        }
        const_reverse_iterator rcend () noexcept {
            return const_reverse_iterator{this->begin ()};
        }
        ///@}

        /// \name Modifiers
        ///@{

        /// Removes all elements from the container.
        /// Invalidates any references, pointers, or iterators referring to contained elements. Any
        /// past-the-end iterators are also invalidated.
        /// Unlike std::vector, the capacity of the small_vector will be reset to BodyElements.
        void clear () noexcept;

        /// Adds an element to the end.
        void push_back (ElementType const & v);

        template <typename InputIt>
        void assign (InputIt first, InputIt last);
        void assign (std::initializer_list<ElementType> ilist) {
            this->assign (std::begin (ilist), std::end (ilist));
        }

        /// Add the specified range to the end of the small_vector.
        template<typename InputIt>
        void append (InputIt first, InputIt last);
        void append (std::initializer_list<ElementType> ilist) {
            this->append (std::begin (ilist), std::end (ilist));
        }

        ///@}

    private:
        /// The actual number of elements for which this buffer is sized.
        /// Note that this may be less than BodyElements.
        std::size_t elements_;

        /// A "small" in-object buffer that is used for relatively small
        /// allocations.
        std::array<ElementType, BodyElements> small_buffer_;

        /// A (potentially) large buffer that is used to satify requests for
        /// buffer element counts that are too large for small_buffer_.
        std::vector<ElementType> big_buffer_;

        /// Will point to either small_buffer_.data() or big_buffer_.data()
        ElementType * buffer_;

        /// Returns true if the given number of elements will fit in the space
        /// allocated for the "small" in-object buffer.
        static constexpr bool is_small (std::size_t elements) {
            return elements <= BodyElements;
        }

        ElementType * set_buffer_ptr (std::size_t required_elements) {
            buffer_ = is_small (required_elements) ? small_buffer_.data () : big_buffer_.data ();
            return buffer_;
        }
    };

    // (ctor)
    // ~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    small_vector<ElementType, BodyElements>::small_vector (std::size_t required_elements)
            : elements_ (required_elements)
            , buffer_ (nullptr) {

        if (!is_small (required_elements)) {
            big_buffer_.resize (required_elements);
        }

        this->set_buffer_ptr (required_elements);
    }

    template <typename ElementType, std::size_t BodyElements>
    small_vector<ElementType, BodyElements>::small_vector ()
            : small_vector (0) {}

    template <typename ElementType, std::size_t BodyElements>
    small_vector<ElementType, BodyElements>::small_vector (small_vector && other)
            : elements_ (std::move (other.elements_))
            , small_buffer_ (std::move (other.small_buffer_))
            , big_buffer_ (std::move (other.big_buffer_))
            , buffer_ (nullptr) {

        this->set_buffer_ptr (elements_);
    }

    template <typename ElementType, std::size_t BodyElements>
    small_vector<ElementType, BodyElements>::small_vector (small_vector const & rhs)
            : small_vector (rhs.size ()) {
        std::copy (std::begin (rhs), std::end (rhs), this->begin ());
    }

    // operator=
    // ~~~~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    auto small_vector<ElementType, BodyElements>::operator= (small_vector && other)
        -> small_vector & {
        elements_ = std::move (other.elements_);
        small_buffer_ = std::move (other.small_buffer_);
        big_buffer_ = std::move (other.big_buffer_);

        this->set_buffer_ptr (elements_);
        return *this;
    }

    // resize
    // ~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    void small_vector<ElementType, BodyElements>::resize (std::size_t new_elements) {
        if (new_elements != elements_) {
            bool const is_small_before = is_small (elements_);
            bool const is_small_after = is_small (new_elements);

            big_buffer_.resize (is_small_after ? 0 : new_elements);

            // Update the buffer pointer and preserve the contents if we've switched from small to larger buffer or vice-versa.
            auto old_buffer = buffer_;
            auto new_buffer = this->set_buffer_ptr (new_elements);

            if (is_small_before != is_small_after) {
                std::copy (old_buffer, old_buffer + elements_, new_buffer);
            }

            elements_ = new_elements;
        }
    }

    // clear
    // ~~~~~
    template <typename ElementType, std::size_t BodyElements>
    void small_vector<ElementType, BodyElements>::clear () noexcept {
        big_buffer_.clear ();
        elements_ = 0;
        this->set_buffer_ptr (elements_);
    }

    // push_back
    // ~~~~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    void small_vector<ElementType, BodyElements>::push_back (ElementType const & v) {
        auto new_elements = elements_ + 1U;
        bool const is_small_before = is_small (elements_);
        bool const is_small_after = is_small (new_elements);
        if (is_small_after) {
            small_buffer_ [elements_] = v;
        } else {
            if (is_small_before != is_small_after) {
                big_buffer_.clear ();
                std::copy (buffer_, buffer_ + elements_, std::back_inserter (big_buffer_));
            }
            big_buffer_.push_back (v);
            this->set_buffer_ptr (new_elements);
        }
        ++elements_;
    }

    // assign
    // ~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    template <typename InputIt>
    void small_vector<ElementType, BodyElements>::assign (InputIt first, InputIt last) {
        this->clear ();
        for (; first != last; ++first)
            this->push_back (*first);
    }

    // append
    // ~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    template<typename Iterator>
    void small_vector<ElementType, BodyElements>::append (Iterator first, Iterator last) {
        for (; first != last; ++first) {
            this->push_back (*first);
        }
    }

} // end namespace pstore
#endif // PSTORE_SMALL_VECTOR_HPP
// eof: include/pstore_support/small_vector.hpp
