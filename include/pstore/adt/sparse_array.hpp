//===- include/pstore/adt/sparse_array.hpp ----------------*- mode: C++ -*-===//
//*                                                              *
//*  ___ _ __   __ _ _ __ ___  ___    __ _ _ __ _ __ __ _ _   _  *
//* / __| '_ \ / _` | '__/ __|/ _ \  / _` | '__| '__/ _` | | | | *
//* \__ \ |_) | (_| | |  \__ \  __/ | (_| | |  | | | (_| | |_| | *
//* |___/ .__/ \__,_|_|  |___/\___|  \__,_|_|  |_|  \__,_|\__, | *
//*     |_|                                               |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file sparse_array.hpp
/// \brief Implements a sparse array type.

#ifndef PSTORE_ADT_SPARSE_ARRAY_HPP
#define PSTORE_ADT_SPARSE_ARRAY_HPP

#include <numeric>

#include "pstore/support/bit_count.hpp"
#include "pstore/support/error.hpp"

namespace pstore {

    namespace details {

        /// \tparam Iterator  An iterator type whose value_type is a std::pair<>.
        /// \tparam Accessor  The type of a function which will be called to yield the
        ///   values from the pair. It should accept a single parameter of type Iterator
        ///   and return the value of the first or second field of the referenced
        ///   std::pair<> instance.
        template <typename Iterator, typename Accessor>
        class pair_field_iterator {
            using return_type = typename std::result_of<Accessor (Iterator)>::type;
            static_assert (std::is_reference<return_type>::value,
                           "return type from Accessor must be a reference");

        public:
            using value_type = typename std::remove_reference<return_type>::type;
            using pointer = value_type *;
            using const_pointer = value_type const *;
            using reference = value_type &;
            using const_reference = value_type const &;

            using difference_type = typename std::iterator_traits<Iterator>::difference_type;
            using iterator_category = typename std::iterator_traits<Iterator>::iterator_category;

            pair_field_iterator (Iterator it, Accessor acc)
                    : it_{std::move (it)}
                    , acc_{std::move (acc)} {}

            pair_field_iterator & operator+= (difference_type rhs) {
                it_ += rhs;
                return *this;
            }
            pair_field_iterator & operator-= (difference_type rhs) {
                it_ -= rhs;
                return *this;
            }

            difference_type operator- (pair_field_iterator const & other) const {
                return it_ - other.it_;
            }

            pair_field_iterator operator- (difference_type rhs) {
                pair_field_iterator res{*this};
                it_ -= rhs;
                return res;
            }
            pair_field_iterator operator+ (difference_type rhs) {
                pair_field_iterator res{*this};
                it_ += rhs;
                return res;
            }

            pair_field_iterator & operator++ () {
                it_++;
                return *this;
            }
            pair_field_iterator operator++ (int) {
                auto ret_val = *this;
                ++(*this);
                return ret_val;
            }
            bool operator< (pair_field_iterator const & other) const { return it_ < other.it_; }
            bool operator== (pair_field_iterator const & other) const {
                return it_ == other.it_ && acc_ == other.acc_;
            }
            bool operator!= (pair_field_iterator const & other) const { return !(*this == other); }

            const_reference operator* () const {
                // Delegate the actual deferencing of the iterator to the accessor function.
                return acc_ (it_);
            }
            const_pointer operator-> () const {
                // Delegate the actual deferencing of the iterator to the accessor function.
                // Since we statically assert that the return type of acc_ is a reference,
                // we can take the address of its returned value.
                return &acc_ (it_);
            }

        private:
            Iterator it_;
            /// An accessor function which will be used to extract one of the field
            /// members from the referenced value.
            Accessor acc_;
        };

        // TODO: first_accessor/second_accessor() was originally a lambda defined inside
        // make_pair_field_iterator() but this is rejected by VS2017. Could this be better done
        // with std::get<>()?
        /// Returns a reference to the 'first' field from the value produced by
        /// dereferencing iterator 'it'.
        template <typename Iterator>
        inline auto first_accessor (Iterator it)
            -> decltype (std::iterator_traits<Iterator>::value_type::first) const & {
            return it->first;
        }

        /// Returns a reference to the 'second' field from the value produce by
        /// dereferencing iterator 'it'.
        template <typename Iterator>
        inline auto second_accessor (Iterator it)
            -> decltype (std::iterator_traits<Iterator>::value_type::second) const & {
            return it->second;
        }

        template <typename Iterator, typename Accessor>
        auto make_pair_field_iterator (Iterator it, Accessor acc)
            -> pair_field_iterator<Iterator, Accessor> {
            return {it, acc};
        }

    } // end namespace details

    /// \brief A sparse array type.
    ///
    /// A sparse array implementation which uses a bitmap value (whose type is given by the
    /// BitmapType template parameter) to manage the collection of members. Each position in that
    /// bitmap represents the presence or absence of a value at the corresponding index. A bitmap
    /// value of 1 would mean that a single element at index 0 is present; a bitmap value of 0b101
    /// indices that members 0 and 2 are available. The array members are contiguous. The position
    /// of a specific index can be computed as: `P(v & ((1 << x) - 1))` where P is the population
    /// count function, v is the bitmap-value, and x is the required index.
    template <typename ValueType, typename BitmapType = std::uint64_t>
    class sparse_array {
        template <typename V, typename B>
        friend bool operator== (sparse_array<V, B> const & lhs, sparse_array<V, B> const & rhs);

    public:
        using bitmap_type = BitmapType;

        using value_type = ValueType;
        using size_type = std::size_t;
        using reference = value_type &;
        using const_reference = value_type const &;
        using iterator = ValueType *;
        using const_iterator = ValueType const *;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        /// Constructs a sparse array whose available indices are defined by the iterator range from
        /// [first_index,last_index) and the values assigned to those indices are given by
        /// [FirstValue, LastValue). If the number of elements in [first_value, last_value) is less
        /// than the number of elements in [first_index,last_index), the remaining values are
        /// default-constructed; if it is greater then the remaining values are ignored.
        template <typename IteratorIdx, typename IteratorV>
        sparse_array (IteratorIdx first_index, IteratorIdx last_index, IteratorV first_value,
                      IteratorV last_value);

        /// Constructs a sparse array whose available indices are defined by the iterator range from
        /// [first_index,last_index) and whose corresponding values are default constructed.
        template <typename IteratorIdx>
        sparse_array (IteratorIdx first_index, IteratorIdx last_index);

        sparse_array (sparse_array const &) = delete;
        sparse_array (sparse_array &&) = delete;

        ~sparse_array ();

        sparse_array & operator= (sparse_array const &) = delete;
        sparse_array & operator= (sparse_array &&) = delete;


        /// Constructs a sparse_array<> instance where the indices are extracted from an iterator
        /// range [first_index, last_index) and the values at each index are given by the range
        /// [first_value,last_value). The number of elements in each range must be the same.
        template <typename IteratorIdx, typename IteratorV>
        static auto make_unique (IteratorIdx first_index, IteratorIdx last_index,
                                 IteratorV first_value, IteratorV last_value)
            -> std::unique_ptr<sparse_array>;

        template <typename IteratorIdx>
        static auto make_unique (IteratorIdx first_index, IteratorIdx last_index,
                                 std::initializer_list<ValueType> values)
            -> std::unique_ptr<sparse_array>;

        template <typename Iterator>
        static auto make_unique (Iterator begin, Iterator end) -> std::unique_ptr<sparse_array>;

        static auto make_unique (std::initializer_list<size_type> indices,
                                 std::initializer_list<ValueType> values = {})
            -> std::unique_ptr<sparse_array>;

        static auto make_unique (std::initializer_list<std::pair<size_type, ValueType>> v)
            -> std::unique_ptr<sparse_array> {
            return make_unique (std::begin (v), std::end (v));
        }

        /// \name Capacity
        ///@{
        constexpr bool empty () const noexcept { return bitmap_ == 0U; }
        constexpr size_type size () const noexcept { return bit_count::pop_count (bitmap_); }

        /// Returns the maximum number of indices that could be contained by an instance of this
        /// sparse_array type.
        static constexpr size_type max_size () noexcept { return sizeof (BitmapType) * 8; }

        /// Returns true if the sparse array has an index 'pos'.
        bool has_index (size_type pos) const noexcept {
            return pos < max_size () ? (bitmap_ & (BitmapType{1U} << pos)) != 0U : false;
        }
        ///@}

        /// \name Iterators
        ///@{
        /// Returns an iterator to the beginning of the container.
        iterator begin () { return data (); }
        const_iterator begin () const { return data (); }
        const_iterator cbegin () const { return begin (); }

        iterator end () { return begin () + size (); }
        const_iterator end () const { return begin () + size (); }
        const_iterator cend () const { return end (); }

        reverse_iterator rbegin () { return reverse_iterator{end ()}; }
        const_reverse_iterator rbegin () const { return const_reverse_iterator{end ()}; }
        const_reverse_iterator crbegin () const { return rbegin (); }

        reverse_iterator rend () { return reverse_iterator{begin ()}; }
        const_reverse_iterator rend () const { return const_reverse_iterator{begin ()}; }
        const_reverse_iterator crend () const { return rend (); }

        class indices {
        public:
            class const_iterator {
            public:
                using iterator_category = std::forward_iterator_tag;
                using value_type = std::size_t;
                using difference_type = std::ptrdiff_t;
                using pointer = value_type const *;
                using reference = value_type const &;

                constexpr explicit const_iterator (BitmapType bitmap) noexcept
                        : bitmap_{bitmap} {
                    next ();
                }

                constexpr bool operator== (const_iterator const & rhs) const noexcept {
                    return bitmap_ == rhs.bitmap_;
                }
                constexpr bool operator!= (const_iterator const & rhs) const noexcept {
                    return !operator== (rhs);
                }

                constexpr reference operator* () const noexcept { return pos_; }
                constexpr pointer operator-> () const noexcept { return pos_; }

                const_iterator & operator++ () noexcept {
                    bitmap_ >>= 1;
                    ++pos_;
                    next ();
                    return *this;
                }

                const_iterator operator++ (int) noexcept {
                    auto prev = *this;
                    next ();
                    return prev;
                }

                const_iterator operator+ (unsigned x) const {
                    auto result = *this;
                    std::advance (result, x);
                    return result;
                }

            private:
                void next () noexcept {
                    for (; bitmap_ != 0U && (bitmap_ & 1U) == 0U; bitmap_ >>= 1) {
                        ++pos_;
                    }
                }
                BitmapType bitmap_;
                std::size_t pos_ = 0;
            };
            explicit indices (sparse_array const & arr)
                    : bitmap_{arr.bitmap_} {}
            const_iterator begin () const { return const_iterator{bitmap_}; }
            const_iterator end () const { return const_iterator{0U}; }

            constexpr bool empty () const noexcept { return bitmap_ == 0U; }
            /// Returns the index of the first element in the container. This is the smallest
            /// value that can be passed to operator[]. There must be at least one element in
            /// the container.
            unsigned front () const noexcept {
                PSTORE_ASSERT (!empty ());
                return bit_count::ctz (bitmap_);
            }
            /// Returns the index of the last element in the container. This is the largest
            /// value that can be passed to operator[]. There must be at least one element in
            /// the container.
            unsigned back () const noexcept {
                PSTORE_ASSERT (!empty ());
                return sizeof (bitmap_) * 8U - bit_count::clz (bitmap_) - 1U;
            }


        private:
            BitmapType const bitmap_;
        };

        indices get_indices () const noexcept { return indices{*this}; }

        ///@}

        /// \name Element access
        ///@{

        ValueType * data () noexcept { return &elements_[0]; }
        ValueType const * data () const noexcept { return &elements_[0]; }

        reference operator[] (size_type pos) noexcept {
            return sparse_array::index_impl (*this, pos);
        }
        const_reference operator[] (size_type pos) const noexcept {
            return sparse_array::index_impl (*this, pos);
        }

        reference at (size_type pos) { return sparse_array::at_impl (*this, pos); }
        const_reference at (size_type pos) const { return sparse_array::at_impl (*this, pos); }

        /// Returns a reference to the first element in the container. Calling front() on an
        /// empty container is undefined.
        reference front () {
            PSTORE_ASSERT (!empty ());
            return (*this)[get_indices ().front ()];
        }
        /// Returns a reference to the first element in the container. Calling front() on an
        /// empty container is undefined.
        const_reference front () const {
            PSTORE_ASSERT (!empty ());
            return (*this)[get_indices ().front ()];
        }
        /// Returns a reference to the last element in the container. Calling back() on an empty
        /// container is undefined.
        reference back () {
            PSTORE_ASSERT (!empty ());
            return (*this)[get_indices ().back ()];
        }
        /// Returns a reference to the last element in the container. Calling back() on an empty
        /// container is undefined.
        const_reference back () const {
            PSTORE_ASSERT (!empty ());
            return (*this)[get_indices ().back ()];
        }

        ///@}

        void fill (ValueType const & value) { std::fill_n (begin (), size (), value); }

        constexpr std::size_t size_bytes () const {
            return sparse_array::size_bytes (this->size ());
        }
        static constexpr std::size_t size_bytes (std::size_t const num_entries) noexcept {
            static_assert (sizeof (elements_) / sizeof (ValueType) == 1U,
                           "Expected elements_ to be an array of 1 ValueType");
            return sizeof (sparse_array) +
                   (std::max (std::size_t{1}, num_entries) - 1U) * sizeof (ValueType);
        }

        void * operator new (std::size_t const /*count*/, void * const ptr) { return ptr; }
        void operator delete (void * const /*ptr*/, void * const /*place*/) {}
        void operator delete (void * const p) { ::operator delete (p); }

    private:
        /// Computes the number of bytes of storage that are required for a sparse_array.
        ///
        /// \tparam InputIterator  An iterator type which, when dereferenced, will produce an array
        ///   index.
        /// \param count  The number of bytes to allocate (to which the storage
        ///   required for the array elements will be added).
        /// \param first  The beginning of the range of index values.
        /// \param last  The end of the range of index values.
        /// \returns The number of bytes of storage that are required for a spare_array with the
        /// indices described by [first, last).
        template <typename InputIterator>
        static std::size_t required_bytes (std::size_t const count, InputIterator const first,
                                           InputIterator const last) {
            // There will always be sufficient storage for at least 1 instance of
            // ValueType (this comes from the built-in array).
            static_assert (sizeof (elements_) / sizeof (ValueType) == 1U,
                           "Expected elements_ to be an array of 1 ValueType");
            auto const elements = std::max (bit_count::pop_count (bitmap (first, last)), 1U);
            return count - sizeof (elements_) + elements * sizeof (ValueType);
        }

        /// \tparam InputIterator  An iterator type which, when dereferenced, will produce an array
        ///   index.
        /// \param count  The number of bytes to allocate (to which the storage
        ///   required for the array elements will be added).
        /// \param indices_first  The beginning of the range of index values.
        /// \param indices_last  The end of the range of index values.
        /// \returns A pointer to the newly allocated memory block.
        template <typename InputIterator>
        void * operator new (std::size_t count, InputIterator indices_first,
                             InputIterator indices_last) {
            return ::operator new (
                sparse_array::required_bytes (count, indices_first, indices_last));
        }

        /// Placement delete member function to match the placement new member
        /// function which takes two input iterators.
        template <typename InputIterator>
        void operator delete (void * const p, InputIterator const /*indices_first*/,
                              InputIterator const /*indices_last*/) {
            return ::operator delete (p);
        }

        BitmapType const bitmap_;
        ValueType elements_[1];

        /// Computes the bitmap value given a pair of iterators which will produce the
        /// sequence of indices to be present in a sparse array.
        template <typename InputIterator>
        static BitmapType bitmap (InputIterator first, InputIterator last);

        /// The implementation of operator[].
        template <typename SparseArray,
                  typename ResultType = typename inherit_const<SparseArray, ValueType>::type>
        static ResultType & index_impl (SparseArray && sa, size_type pos) noexcept;

        /// The implementation of at().
        template <typename SparseArray,
                  typename ResultType = typename inherit_const<SparseArray, ValueType>::type>
        static ResultType & at_impl (SparseArray && sa, size_type pos);
    };

    // (ctor)
    // ~~~~~~
    template <typename ValueType, typename BitmapType>
    template <typename IteratorIdx, typename IteratorV>
    sparse_array<ValueType, BitmapType>::sparse_array (IteratorIdx first_index,
                                                       IteratorIdx last_index,
                                                       IteratorV first_value, IteratorV last_value)
            : bitmap_{bitmap (first_index, last_index)} {

        // Deal with the first element. This is the odd-one-out becuase it's in the
        // declaration of the object and will have been default-constructed by the
        // compiler.
        if (first_index != last_index) {
            if (first_value != last_value) {
                elements_[0] = *(first_value++);
            }
            ++first_index;
        }

        auto out = &elements_[1];

        // Now construct any remaining elements into the uninitialized memory past the
        // end of the object using placement new.
        for (; first_index != last_index && first_value != last_value;
             ++first_index, ++first_value, ++out) {
            new (out) ValueType (*first_value);
        }
        // Default-construct any remaining objects for which we don't have a value.
        for (; first_index != last_index; ++first_index, ++out) {
            new (out) ValueType ();
        }
    }

    template <typename ValueType, typename BitmapType>
    template <typename IteratorIdx>
    sparse_array<ValueType, BitmapType>::sparse_array (IteratorIdx first_index,
                                                       IteratorIdx last_index)
            : bitmap_{bitmap (first_index, last_index)} {}

    // (dtor)
    // ~~~~~~
    template <typename ValueType, typename BitmapType>
    sparse_array<ValueType, BitmapType>::~sparse_array () {
        auto const elements = size ();
        if (elements > 1) {
            for (auto it = &elements_[1], end = &elements_[0] + elements; it != end; ++it) {
                it->~ValueType ();
            }
        }
    }

    // make unique
    // ~~~~~~~~~~~
    template <typename ValueType, typename BitmapType>
    template <typename Iterator>
    auto sparse_array<ValueType, BitmapType>::make_unique (Iterator begin, Iterator end)
        -> std::unique_ptr<sparse_array> {

        auto begin_first =
            details::make_pair_field_iterator (begin, details::first_accessor<Iterator>);
        auto end_first = details::make_pair_field_iterator (end, details::first_accessor<Iterator>);
        auto begin_second =
            details::make_pair_field_iterator (begin, details::second_accessor<Iterator>);
        auto end_second =
            details::make_pair_field_iterator (end, details::second_accessor<Iterator>);
        return std::unique_ptr<sparse_array<ValueType, BitmapType>>{
            new (begin_first, end_first) sparse_array<ValueType, BitmapType> (
                begin_first, end_first, begin_second, end_second)};
    }

    template <typename ValueType, typename BitmapType>
    template <typename IteratorIdx, typename IteratorV>
    auto sparse_array<ValueType, BitmapType>::make_unique (IteratorIdx first_index,
                                                           IteratorIdx last_index,
                                                           IteratorV first_value,
                                                           IteratorV last_value)
        -> std::unique_ptr<sparse_array> {

        return std::unique_ptr<sparse_array<ValueType, BitmapType>>{
            new (first_index, last_index) sparse_array<ValueType, BitmapType> (
                first_index, last_index, first_value, last_value)};
    }

    template <typename ValueType, typename BitmapType>
    auto sparse_array<ValueType, BitmapType>::make_unique (std::initializer_list<size_type> indices,
                                                           std::initializer_list<ValueType> values)
        -> std::unique_ptr<sparse_array> {

        return make_unique (std::begin (indices), std::end (indices), std::begin (values),
                            std::end (values));
    }

    template <typename ValueType, typename BitmapType>
    template <typename IteratorIdx>
    auto sparse_array<ValueType, BitmapType>::make_unique (IteratorIdx first_index,
                                                           IteratorIdx last_index,
                                                           std::initializer_list<ValueType> values)
        -> std::unique_ptr<sparse_array> {

        return std::unique_ptr<sparse_array<ValueType, BitmapType>>{
            new (first_index, last_index) sparse_array<ValueType, BitmapType> (
                first_index, last_index, std::begin (values), std::end (values))};
    }

    // bitmap
    // ~~~~~~
    template <typename ValueType, typename BitmapType>
    template <typename InputIterator>
    BitmapType sparse_array<ValueType, BitmapType>::bitmap (InputIterator first,
                                                            InputIterator last) {
        using iter_value_type = typename std::iterator_traits<InputIterator>::value_type;
        auto op = [] (BitmapType mm, iter_value_type v) {
            auto const idx = static_cast<unsigned> (v);
            PSTORE_ASSERT (idx < max_size ());
            auto const mask = BitmapType{1U} << idx;
            PSTORE_ASSERT ((mm & mask) == 0U &&
                           "The same index must not appear more than once in the "
                           "collection of sparse indices");
            return static_cast<BitmapType> (mm | mask);
        };
        return std::accumulate (first, last, BitmapType{0U}, op);
    }

    // index impl
    // ~~~~~~~~~~
    template <typename ValueType, typename BitmapType>
    template <typename SparseArray, typename ResultType>
    ResultType & sparse_array<ValueType, BitmapType>::index_impl (SparseArray && sa,
                                                                  size_type pos) noexcept {
        PSTORE_ASSERT (pos < max_size ());
        auto mask = BitmapType{1U} << pos;
        PSTORE_ASSERT ((sa.bitmap_ & mask) != 0U);
        mask--;
        return sa.elements_[bit_count::pop_count (static_cast<BitmapType> (sa.bitmap_ & mask))];
    }

    // at impl
    // ~~~~~~~
    template <typename ValueType, typename BitmapType>
    template <typename SparseArray, typename ResultType>
    ResultType & sparse_array<ValueType, BitmapType>::at_impl (SparseArray && sa, size_type pos) {
        if (pos < max_size ()) {
            auto const bit_position = BitmapType{1} << pos;
            if ((sa.bitmap_ & bit_position) != 0) {
                return sa.elements_[bit_count::pop_count (sa.bitmap_ & (bit_position - 1U))];
            }
        }
        raise_exception (std::out_of_range ("spare array index out of range"));
    }


    // operator==
    // ~~~~~~~~~~
    template <typename ValueType, typename BitmapType>
    inline bool operator== (sparse_array<ValueType, BitmapType> const & lhs,
                            sparse_array<ValueType, BitmapType> const & rhs) {
        return lhs.bitmap_ == rhs.bitmap_ &&
               std::equal (std::begin (lhs), std::end (lhs), std::begin (rhs));
    }

    // operator!=
    // ~~~~~~~~~~
    template <typename ValueType, typename BitmapType>
    inline bool operator!= (sparse_array<ValueType, BitmapType> const & lhs,
                            sparse_array<ValueType, BitmapType> const & rhs) {
        return !(lhs == rhs);
    }

} // end namespace pstore

namespace std {

    template <std::size_t Ip, typename ValueType, typename BitmapType>
    constexpr ValueType & get (pstore::sparse_array<ValueType, BitmapType> & arr) noexcept {
        static_assert (Ip < pstore::sparse_array<ValueType, BitmapType>::max_size (),
                       "Index out of bounds in std::get<> (sparse_array)");
        return arr[Ip];
    }

    template <std::size_t Ip, typename ValueType, typename BitmapType>
    constexpr ValueType const &
    get (pstore::sparse_array<ValueType, BitmapType> const & arr) noexcept {
        static_assert (Ip < pstore::sparse_array<ValueType, BitmapType>::max_size (),
                       "Index out of bounds in std::get<> (const sparse_array)");
        return arr[Ip];
    }

} // end namespace std

#endif // PSTORE_ADT_SPARSE_ARRAY_HPP
