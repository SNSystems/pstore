//*       _                 _            _                  _              *
//*   ___| |__  _   _ _ __ | | _____  __| | __   _____  ___| |_ ___  _ __  *
//*  / __| '_ \| | | | '_ \| |/ / _ \/ _` | \ \ / / _ \/ __| __/ _ \| '__| *
//* | (__| | | | |_| | | | |   <  __/ (_| |  \ V /  __/ (__| || (_) | |    *
//*  \___|_| |_|\__,_|_| |_|_|\_\___|\__,_|   \_/ \___|\___|\__\___/|_|    *
//*                                                                        *
//===- include/pstore/adt/chunked_vector.hpp ------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_ADT_CHUNKED_VECTOR_HPP
#define PSTORE_ADT_CHUNKED_VECTOR_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <list>

namespace pstore {

    /// Chunked-vector is a sequence-container which uses a list of large blocks ("chunks") to
    /// ensure very fast append times at the expense of only permitting bi-directional iterators:
    /// random access is not supported, unlike std::deque<> or std::vector<>.
    ///
    /// \tparam T The type of the elements.
    /// \tparam ElementsPerChunk The number of elements in an individual chunk.
    /// \tparam ActualSize The storage allocated to an individual element. Normally equal to
    ///   sizeof(T), this can be increased to allow for dynamically-sized types.
    /// \tparam ActualAlign The alignment of an individual element. Normally equal to alignof(T).
    template <typename T,
              std::size_t ElementsPerChunk = std::max (4096 / sizeof (T), std::size_t{1}),
              std::size_t ActualSize = sizeof (T), std::size_t ActualAlign = alignof (T)>
    class chunked_vector {
        static_assert (ElementsPerChunk > 0, "Must be at least 1 element per chunk");
        static_assert (ActualSize >= sizeof (T), "ActualSize must be at least sizeof(T)");
        static_assert (ActualAlign >= alignof (T), "ActualAlign must be at least alignof(T)");

        class chunk;
        using chunk_list = std::list<chunk>;
        template <bool Const = true>
        class iterator_base;

    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type &;
        using const_reference = value_type const &;
        using pointer = T *;
        using const_pointer = T const *;

        using iterator = iterator_base<false>;
        using const_iterator = iterator_base<true>;
        // using reverse_iterator =
        // using const_reverse_iterator =

        chunked_vector () = default;
        chunked_vector (chunked_vector const &) = delete;
        chunked_vector (chunked_vector && other) noexcept
                : chunks_{std::move (other.chunks_)}
                , size_{other.size_} {
            other.size_ = 0;
        }

        ~chunked_vector () noexcept { this->clear (); }

        constexpr bool empty () const noexcept { return size_ == 0; }
        constexpr size_type size () const noexcept { return size_; }

        iterator begin () noexcept { return {chunks_.begin (), 0U}; }
        iterator end () noexcept { return {chunks_.end (), 0U}; }
        const_iterator begin () const noexcept { return {chunks_.begin (), 0U}; }
        const_iterator end () const noexcept { return {chunks_.end (), 0U}; }
        const_iterator cbegin () const noexcept { return begin (); }
        const_iterator cend () const noexcept { return end (); }

        void clear () noexcept {
            chunks_.clear ();
            size_ = 0;
        }
        void reserve (std::size_t /*size*/) {
            // TODO: Not currently implemented.
        }
        std::size_t capacity () const noexcept { return chunks_.size () * ElementsPerChunk; }

        template <typename... Args>
        reference emplace_back (Args &&... args);

        reference push_back (T const & value) { return emplace_back (value); }
        reference push_back (T && value) { return emplace_back (std::move (value)); }

        /// Returns a reference to the first element in the container. Calling front
        /// on an empty container is undefined.
        reference front () {
            assert (size_ > 0);
            return chunks_.front ().front ();
        }
        /// Returns a reference to the first element in the container. Calling front
        /// on an empty container is undefined.
        const_reference front () const {
            assert (size_ > 0);
            return chunks_.front ().front ();
        }

        /// Returns a reference to the last element in the container. Calling back
        /// on an empty container is undefined.
        reference back () {
            assert (size_ > 0);
            return chunks_.back ().back ();
        }
        /// Returns a reference to the last element in the container. Calling back
        /// on an empty container is undefined.
        const_reference back () const {
            assert (size_ > 0);
            return chunks_.back ().back ();
        }

        void swap (chunked_vector & other) noexcept {
            std::swap (chunks_, other.chunks_);
            std::swap (size_, other.size_);
        }

        void splice (chunked_vector && other) {
            size_ += other.size ();
            chunks_.splice (chunks_.end (), std::move (other.chunks_));
        }

    private:
        chunk_list chunks_;
        size_type size_ = 0; ///< The number of elements.
    };

    namespace details {

        /// Yields either 'T' or 'T const' depending on the value is IsConst.
        template <typename T, bool IsConst>
        struct value_type {
            using type = typename std::conditional<IsConst, T const, T>::type;
        };

    } // end namespace details

    // emplace_back
    // ~~~~~~~~~~~~
    template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
              std::size_t ActualAlign>
    template <typename... Args>
    auto
    chunked_vector<T, ElementsPerChunk, ActualSize, ActualAlign>::emplace_back (Args &&... args)
        -> reference {
        if (chunks_.size () == 0U || chunks_.back ().size () >= ElementsPerChunk) {
            // Append a new chunk.
            chunks_.emplace_back ();
        }
        // Append a new instance to the last chunk.
        reference result = chunks_.back ().emplace_back (std::forward<Args> (args)...);
        ++size_;
        return result;
    }

    //*     _             _    *
    //*  __| |_ _  _ _ _ | |__ *
    //* / _| ' \ || | ' \| / / *
    //* \__|_||_\_,_|_||_|_\_\ *
    //*                        *
    template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
              std::size_t ActualAlign>
    class chunked_vector<T, ElementsPerChunk, ActualSize, ActualAlign>::chunk {
    public:
        chunk () noexcept = default;
        ~chunk () noexcept;

        // no copying or assignment.
        chunk (chunk const &) = delete;
        bool operator= (chunk const &) = delete;

        T * data () noexcept { return membs_.data (); }
        T const * data () const noexcept { return membs_.data (); }

        T & operator[] (std::size_t index) noexcept {
            assert (index < ElementsPerChunk);
            return reinterpret_cast<T &> (membs_[index]);
        }
        T const & operator[] (std::size_t index) const noexcept {
            assert (index < ElementsPerChunk);
            return reinterpret_cast<T const &> (membs_[index]);
        }
        std::size_t size () const noexcept { return size_; }

        reference front () { return (*this)[0]; }
        const_reference front () const { return (*this)[0]; }
        reference back () { return (*this)[size_ - 1U]; }
        const_reference back () const { return (*this)[size_ - 1U]; }

        template <typename... Args>
        reference emplace_back (Args &&... args);

    private:
        std::size_t size_ = 0U;
        std::array<typename std::aligned_storage<ActualSize, ActualAlign>::type, ElementsPerChunk>
            membs_;
    };

    // dtor
    // ~~~~
    template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
              std::size_t ActualAlign>
    chunked_vector<T, ElementsPerChunk, ActualSize, ActualAlign>::chunk::~chunk () noexcept {
        for (auto ctr = std::size_t{0}; ctr < size_; ++ctr) {
            (*this)[ctr].~T ();
        }
        size_ = 0U;
    }

    // emplace_back
    // ~~~~~~~~~~~~
    template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
              std::size_t ActualAlign>
    template <typename... Args>
    auto chunked_vector<T, ElementsPerChunk, ActualSize, ActualAlign>::chunk::emplace_back (
        Args &&... args) -> reference {
        assert (size_ < membs_.size ());
        T & place = (*this)[size_];
        new (&place) T (std::forward<Args> (args)...);
        ++size_;
        return place;
    }

    //*  _ _                _             _                   *
    //* (_) |_ ___ _ _ __ _| |_ ___ _ _  | |__  __ _ ___ ___  *
    //* | |  _/ -_) '_/ _` |  _/ _ \ '_| | '_ \/ _` (_-</ -_) *
    //* |_|\__\___|_| \__,_|\__\___/_|   |_.__/\__,_/__/\___| *
    //*                                                       *
    template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
              std::size_t ActualAlign>
    template <bool IsConst>
    class chunked_vector<T, ElementsPerChunk, ActualSize, ActualAlign>::iterator_base
            : public std::iterator<std::bidirectional_iterator_tag,
                                   typename details::value_type<T, IsConst>::type> {

        using base = std::iterator<std::bidirectional_iterator_tag,
                                   typename details::value_type<T, IsConst>::type>;
        using list_iterator =
            typename std::conditional<IsConst, typename chunk_list::const_iterator,
                                      typename chunk_list::iterator>::type;

        friend class iterator_base<true>;

    public:
        using typename base::pointer;
        using typename base::reference;

        iterator_base (list_iterator it, std::size_t index)
                : it_{it}
                , index_{index} {}
        iterator_base (iterator_base<false> const & rhs)
                : it_{rhs.it_}
                , index_{rhs.index_} {}
        iterator_base (iterator_base<false> && rhs)
                : it_{std::move (rhs.it_)}
                , index_{std::move (rhs.index_)} {}

        iterator_base & operator= (iterator_base<false> const & rhs) {
            it_ = rhs.it_;
            index_ = rhs.index_;
            return *this;
        }

        iterator_base & operator= (iterator_base<false> && rhs) {
            if (&rhs != this) {
                it_ = std::move (rhs.it_);
                index_ = std::move (rhs.index_);
            }
            return *this;
        }

        bool operator== (iterator_base const & other) const {
            return it_ == other.it_ && index_ == other.index_;
        }
        bool operator!= (iterator_base const & other) const { return !operator== (other); }

        /// Dereference operator
        /// \return the value of the element to which this iterator is currently
        /// pointing
        reference operator* () const noexcept { return (*it_)[index_]; }
        pointer operator-> () const noexcept { return &(*it_)[index_]; }

        iterator_base & operator++ ();
        iterator_base operator++ (int);
        iterator_base & operator-- ();
        iterator_base operator-- (int);

    private:
        list_iterator it_;
        std::size_t index_;
    };

    // Prefix increment
    template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
              std::size_t ActualAlign>
    template <bool IsConst>
    auto chunked_vector<T, ElementsPerChunk, ActualSize,
                        ActualAlign>::iterator_base<IsConst>::operator++ ()
        -> iterator_base<IsConst> & {
        if (++index_ >= it_->size ()) {
            ++it_;
            index_ = 0;
        }
        return *this;
    }

    // Postfix increment operator (e.g., it++)
    template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
              std::size_t ActualAlign>
    template <bool IsConst>
    auto chunked_vector<T, ElementsPerChunk, ActualSize,
                        ActualAlign>::iterator_base<IsConst>::operator++ (int)
        -> iterator_base<IsConst> {
        auto const old = *this;
        ++(*this);
        return old;
    }

    // Prefix decrement
    template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
              std::size_t ActualAlign>
    template <bool IsConst>
    auto chunked_vector<T, ElementsPerChunk, ActualSize,
                        ActualAlign>::iterator_base<IsConst>::operator-- ()
        -> iterator_base<IsConst> & {
        if (index_ > 0U) {
            --index_;
        } else {
            --it_;
            assert (it_->size () > 0);
            index_ = it_->size () - 1U;
        }
        return *this;
    }

    // Postfix decrement
    template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
              std::size_t ActualAlign>
    template <bool IsConst>
    auto chunked_vector<T, ElementsPerChunk, ActualSize,
                        ActualAlign>::iterator_base<IsConst>::operator-- (int)
        -> iterator_base<IsConst> {
        auto old = *this;
        --(*this);
        return old;
    }

} // end namespace pstore

namespace std {

    template <typename T, std::size_t ElementsPerChunk, std::size_t ActualSize,
              std::size_t ActualAlign>
    void
    swap (pstore::chunked_vector<T, ElementsPerChunk, ActualSize, ActualAlign> & lhs,
          pstore::chunked_vector<T, ElementsPerChunk, ActualSize, ActualAlign> & rhs) noexcept {
        lhs.swap (rhs);
    }

} // end namespace std

#endif // PSTORE_ADT_CHUNKED_VECTOR_HPP
