//*                      _ _                  _              *
//*  ___ _ __ ___   __ _| | | __   _____  ___| |_ ___  _ __  *
//* / __| '_ ` _ \ / _` | | | \ \ / / _ \/ __| __/ _ \| '__| *
//* \__ \ | | | | | (_| | | |  \ V /  __/ (__| || (_) | |    *
//* |___/_| |_| |_|\__,_|_|_|   \_/ \___|\___|\__\___/|_|    *
//*                                                          *
//===- include/pstore/adt/small_vector.hpp --------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file small_vector.hpp
/// \brief Provides a small, normally stack allocated, buffer but which can be
/// resized dynamically when necessary.

#ifndef PSTORE_ADT_SMALL_VECTOR_HPP
#define PSTORE_ADT_SMALL_VECTOR_HPP

#include <array>
#include <cstddef>
#include <initializer_list>
#include <vector>

#include "pstore/support/assert.hpp"
#include "pstore/support/inherit_const.hpp"

namespace pstore {

    /// A class which provides a vector-like interface to a small, normally stack
    /// allocated, buffer which may, if necessary, be resized. It is normally used
    /// to contain string buffers where they are typically small enough to be
    /// stack-allocated, but where the code must gracefully suport arbitrary lengths.
    template <typename ElementType, std::size_t BodyElements = 256>
    class small_vector {
    public:
        using value_type = ElementType;

        using reference = value_type &;
        using const_reference = value_type const &;
        using pointer = value_type *;
        using const_pointer = value_type const *;

        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        using iterator = value_type *;
        using const_iterator = value_type const *;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        /// Constructs the buffer with an initial size of 0.
        small_vector () noexcept;
        /// Constructs the buffer with the given initial numbe of elements.
        explicit small_vector (std::size_t required_elements);
        explicit small_vector (std::initializer_list<ElementType> init);
        small_vector (small_vector && other) noexcept;
        small_vector (small_vector const & rhs);

        ~small_vector () noexcept = default;

        small_vector & operator= (small_vector && other) noexcept;
        small_vector & operator= (small_vector const & other);

        /// \name Element access
        ///@{
        ElementType const * data () const noexcept { return buffer_; }
        ElementType * data () noexcept { return buffer_; }

        ElementType const & operator[] (std::size_t n) const noexcept { return index (*this, n); }
        ElementType & operator[] (std::size_t n) noexcept { return index (*this, n); }

        ElementType & back () { return index (*this, size () - 1U); }

        ///@}

        /// \name Capacity
        ///@{
        /// Returns the number of elements.
        std::size_t size () const noexcept { return elements_; }
        std::size_t size_bytes () const noexcept { return elements_ * sizeof (ElementType); }

        /// Checks whether the container is empty.
        bool empty () const noexcept { return elements_ == 0; }

        /// Returns the number of elements that can be held in currently allocated
        /// storage.
        std::size_t capacity () const noexcept {
            return std::max (BodyElements, big_buffer_.capacity ());
        }

        /// Increase the capacity of the vector to a value that's greater or equal to new_cap. If
        /// new_cap is greater than the current capacity(), new storage is allocated, otherwise the
        /// method does nothing. reserve() does not change the size of the vector.
        ///
        /// \note If new_cap is greater than capacity(), all iterators, including the past-the-end
        /// iterator, and all references to the elements are invalidated. Otherwise, no iterators or
        /// references are invalidated.
        ///
        /// \param new_cap The new capacity of the vector
        void reserve (std::size_t new_cap);

        /// Resizes the container so that it is large enough for accommodate the
        /// given number of elements.
        ///
        /// \note Calling this function invalidates the contents of the buffer and
        ///   any iterators.
        ///
        /// \param new_elements The number of elements that the buffer is to
        ///                     accommodate.
        void resize (std::size_t new_elements);
        ///@}

        /// \name Iterators
        ///@{
        /// Returns an iterator to the beginning of the container.
        const_iterator begin () const noexcept { return buffer_; }
        iterator begin () noexcept { return buffer_; }
        const_iterator cbegin () noexcept { return buffer_; }
        /// Returns a reverse iterator to the first element of the reversed
        /// container. It corresponds to the last element of the non-reversed
        /// container.
        reverse_iterator rbegin () noexcept { return reverse_iterator{this->end ()}; }
        const_reverse_iterator rbegin () const noexcept {
            return const_reverse_iterator{this->end ()};
        }
        const_reverse_iterator rcbegin () noexcept { return const_reverse_iterator{this->end ()}; }

        /// Returns an iterator to the end of the container.
        const_iterator end () const noexcept { return buffer_ + elements_; }
        iterator end () noexcept { return buffer_ + elements_; }
        const_iterator cend () noexcept { return buffer_ + elements_; }
        reverse_iterator rend () noexcept { return reverse_iterator{this->begin ()}; }
        const_reverse_iterator rend () const noexcept {
            return const_reverse_iterator{this->begin ()};
        }
        const_reverse_iterator rcend () noexcept { return const_reverse_iterator{this->begin ()}; }
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
        template <typename... Args>
        void emplace_back (Args &&... args);

        template <typename InputIt>
        void assign (InputIt first, InputIt last);

        void assign (std::initializer_list<ElementType> ilist) {
            this->assign (std::begin (ilist), std::end (ilist));
        }

        /// Add the specified range to the end of the small vector.
        template <typename InputIt>
        void append (InputIt first, InputIt last);
        void append (std::initializer_list<ElementType> ilist) {
            this->append (std::begin (ilist), std::end (ilist));
        }

        ///@}

    private:
        /// The const- and non-const implementation of operator[].
        template <typename SmallVector,
                  typename ResultType = typename inherit_const<SmallVector, ElementType>::type>
        static ResultType & index (SmallVector && sm, std::size_t const n) noexcept {
            PSTORE_ASSERT (n < sm.size ());
            return sm.buffer_[n];
        }

        /// The actual number of elements for which this buffer is sized.
        /// Note that this may be less than BodyElements.
        std::size_t elements_ = 0;

        /// A "small" in-object buffer that is used for relatively small
        /// allocations.
        std::array<ElementType, BodyElements> small_buffer_{{}};

        /// A (potentially) large buffer that is used to satify requests for
        /// buffer element counts that are too large for small_buffer_.
        std::vector<ElementType> big_buffer_;

        /// Will point to either small_buffer_.data() or big_buffer_.data()
        ElementType * buffer_ = nullptr;

        /// Returns true if the given number of elements will fit in the space
        /// allocated for the "small" in-object buffer.
        static constexpr bool is_small (std::size_t const elements) noexcept {
            return elements <= BodyElements;
        }

        void switch_to_big (std::size_t new_elements);
        template <typename... Args>
        void emplace_back_big (Args &&... args);
        ElementType * set_buffer_ptr (std::size_t const required_elements) noexcept {
            buffer_ = is_small (required_elements) ? small_buffer_.data () : big_buffer_.data ();
            return buffer_;
        }
    };

    // (ctor)
    // ~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    small_vector<ElementType, BodyElements>::small_vector (std::size_t const required_elements)
            : elements_{required_elements}
            , small_buffer_{} {

        if (!is_small (elements_)) {
            big_buffer_.resize (elements_);
        }
        this->set_buffer_ptr (elements_);
    }

    template <typename ElementType, std::size_t BodyElements>
    small_vector<ElementType, BodyElements>::small_vector () noexcept
            : small_buffer_{} {
        this->set_buffer_ptr (elements_);
    }

    template <typename ElementType, std::size_t BodyElements>
    small_vector<ElementType, BodyElements>::small_vector (small_vector && other) noexcept
            : elements_ (std::move (other.elements_))
            , small_buffer_ (std::move (other.small_buffer_))
            , big_buffer_ (std::move (other.big_buffer_)) {

        this->set_buffer_ptr (elements_);
    }

    template <typename ElementType, std::size_t BodyElements>
    small_vector<ElementType, BodyElements>::small_vector (std::initializer_list<ElementType> init)
            : small_vector () {
        this->reserve (init.size ());
        std::copy (std::begin (init), std::end (init), std::back_inserter (*this));
    }

    template <typename ElementType, std::size_t BodyElements>
    small_vector<ElementType, BodyElements>::small_vector (small_vector const & rhs)
            : small_vector () {
        this->reserve (rhs.size ());
        std::copy (std::begin (rhs), std::end (rhs), std::back_inserter (*this));
    }

    // operator=
    // ~~~~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    auto small_vector<ElementType, BodyElements>::operator= (small_vector && other) noexcept
        -> small_vector & {
        elements_ = std::move (other.elements_);
        small_buffer_ = std::move (other.small_buffer_);
        big_buffer_ = std::move (other.big_buffer_);

        this->set_buffer_ptr (elements_);
        return *this;
    }

    template <typename ElementType, std::size_t BodyElements>
    auto small_vector<ElementType, BodyElements>::operator= (small_vector const & rhs)
        -> small_vector & {
        if (this != &rhs) {
            this->clear ();
            this->resize (rhs.size ());
            std::copy (std::begin (rhs), std::end (rhs), std::begin (*this));
        }
        return *this;
    }

    // reserve
    // ~~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    void small_vector<ElementType, BodyElements>::reserve (std::size_t new_cap) {
        if (new_cap > capacity ()) {
            big_buffer_.reserve (new_cap);
        }
    }

    // resize
    // ~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    void small_vector<ElementType, BodyElements>::resize (std::size_t new_elements) {
        if (new_elements != elements_) {
            bool const is_small_before = is_small (elements_);
            bool const is_small_after = is_small (new_elements);

            big_buffer_.resize (is_small_after ? 0 : new_elements);

            // Update the buffer pointer and preserve the contents if we've switched from small to
            // larger buffer or vice-versa.
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
    inline void small_vector<ElementType, BodyElements>::push_back (ElementType const & v) {
        auto const new_elements = elements_ + 1U;
        if (is_small (new_elements)) {
            small_buffer_[elements_] = v;
        } else {
            if (is_small (elements_)) {
                this->switch_to_big (new_elements);
            }
            big_buffer_.push_back (v);
            this->set_buffer_ptr (new_elements);
        }
        elements_ = new_elements;
    }

    // emplace_back
    // ~~~~~~~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    template <typename... Args>
    inline void small_vector<ElementType, BodyElements>::emplace_back (Args &&... args) {
        auto const new_elements = elements_ + 1U;
        if (!is_small (new_elements)) {
            return emplace_back_big (std::forward<Args> (args)...);
        }
        small_buffer_[elements_] = ElementType (std::forward<Args> (args)...);
        elements_ = new_elements;
    }

    // emplace_back_big
    // ~~~~~~~~~~~~~~~~
    // The "slow" path for emplace_back which inserts into the "big" vector.
    template <typename ElementType, std::size_t BodyElements>
    template <typename... Args>
    void small_vector<ElementType, BodyElements>::emplace_back_big (Args &&... args) {
        auto const new_elements = elements_ + 1U;
        if (is_small (elements_)) {
            this->switch_to_big (new_elements);
        }
        big_buffer_.emplace_back (std::forward<Args> (args)...);
        this->set_buffer_ptr (new_elements);
        elements_ = new_elements;
    }

    // switch_to_big
    // ~~~~~~~~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    void small_vector<ElementType, BodyElements>::switch_to_big (std::size_t new_elements) {
        big_buffer_.clear ();
        big_buffer_.reserve (new_elements);
        std::copy (buffer_, buffer_ + elements_, std::back_inserter (big_buffer_));
        this->set_buffer_ptr (new_elements);
    }

    // assign
    // ~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    template <typename InputIt>
    void small_vector<ElementType, BodyElements>::assign (InputIt first, InputIt last) {
        this->clear ();
        for (; first != last; ++first) {
            this->push_back (*first);
        }
    }

    // append
    // ~~~~~~
    template <typename ElementType, std::size_t BodyElements>
    template <typename Iterator>
    void small_vector<ElementType, BodyElements>::append (Iterator first, Iterator last) {
        for (; first != last; ++first) {
            this->push_back (*first);
        }
    }

    template <typename ElementType, std::size_t LhsBodyElements, std::size_t RhsBodyElements>
    bool operator== (small_vector<ElementType, LhsBodyElements> const & lhs,
                     small_vector<ElementType, RhsBodyElements> const & rhs) {
        return std::equal (std::begin (lhs), std::end (lhs), std::begin (rhs), std::end (rhs));
    }
    template <typename ElementType, std::size_t LhsBodyElements, std::size_t RhsBodyElements>
    bool operator!= (small_vector<ElementType, LhsBodyElements> const & lhs,
                     small_vector<ElementType, RhsBodyElements> const & rhs) {
        return !operator== (lhs, rhs);
    }

} // end namespace pstore
#endif // PSTORE_ADT_SMALL_VECTOR_HPP
