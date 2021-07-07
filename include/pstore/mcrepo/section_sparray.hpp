//===- include/pstore/mcrepo/section_sparray.hpp ----------*- mode: C++ -*-===//
//*                _   _                                                     *
//*  ___  ___  ___| |_(_) ___  _ __    ___ _ __   __ _ _ __ _ __ __ _ _   _  *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  / __| '_ \ / _` | '__| '__/ _` | | | | *
//* \__ \  __/ (__| |_| | (_) | | | | \__ \ |_) | (_| | |  | | | (_| | |_| | *
//* |___/\___|\___|\__|_|\___/|_| |_| |___/ .__/ \__,_|_|  |_|  \__,_|\__, | *
//*                                       |_|                         |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file section_sparray.hpp
/// \brief A wrapper for sparse_array<> which accepts indices of type section_kind.

#ifndef PSTORE_MCREPO_SECTION_SPARRAY_HPP
#define PSTORE_MCREPO_SECTION_SPARRAY_HPP

#include <iterator>
#include <limits>

#include "pstore/adt/sparse_array.hpp"
#include "pstore/mcrepo/section.hpp"

namespace pstore {
    namespace repo {

        namespace details {

            template <typename CastToType, typename BaseIterator>
            class cast_iterator {
            public:
                using iterator_category =
                    typename std::iterator_traits<BaseIterator>::iterator_category;
                using value_type = CastToType;
                using difference_type =
                    typename std::iterator_traits<BaseIterator>::difference_type;
                using pointer = value_type const *;
                using reference = value_type const &;

                constexpr explicit cast_iterator (BaseIterator it);
                constexpr bool operator== (cast_iterator const & rhs) const;
                constexpr bool operator!= (cast_iterator const & rhs) const;
                constexpr reference operator* () const;
                cast_iterator & operator++ ();  // ++prefix
                cast_iterator operator++ (int); // postfix++
                cast_iterator & operator-- ();  // --prefix
                cast_iterator operator-- (int); // postfix--
                cast_iterator operator+ (unsigned x) const { return cast_iterator{it_ + x}; }
                cast_iterator operator- (unsigned x) const { return cast_iterator{it_ - x}; }

            private:
                BaseIterator it_;
                mutable value_type value_;
            };
            template <typename CastToType, typename BaseIterator>
            constexpr cast_iterator<CastToType, BaseIterator>::cast_iterator (BaseIterator it)
                    : it_{it} {}
            template <typename CastToType, typename BaseIterator>
            constexpr bool
            cast_iterator<CastToType, BaseIterator>::operator== (cast_iterator const & rhs) const {
                return it_ == rhs.it_;
            }
            template <typename CastToType, typename BaseIterator>
            constexpr bool
            cast_iterator<CastToType, BaseIterator>::operator!= (cast_iterator const & rhs) const {
                return !operator== (rhs);
            }
            template <typename CastToType, typename BaseIterator>
            constexpr auto cast_iterator<CastToType, BaseIterator>::operator* () const
                -> reference {
                return value_ = static_cast<CastToType> (*it_);
            }
            template <typename CastToType, typename BaseIterator>
            inline auto cast_iterator<CastToType, BaseIterator>::operator++ ()
                -> cast_iterator & { // ++prefix
                ++it_;
                return *this;
            }
            template <typename CastToType, typename BaseIterator>
            inline auto cast_iterator<CastToType, BaseIterator>::operator++ (int)
                -> cast_iterator { // prefix++
                auto prev = *this;
                ++(*this);
                return prev;
            }
            template <typename CastToType, typename BaseIterator>
            inline auto cast_iterator<CastToType, BaseIterator>::operator-- ()
                -> cast_iterator & { // ++prefix
                --it_;
                return *this;
            }
            template <typename CastToType, typename BaseIterator>
            inline auto cast_iterator<CastToType, BaseIterator>::operator-- (int)
                -> cast_iterator { // prefix++
                auto prev = *this;
                --(*this);
                return prev;
            }

        } // end namespace details

        /// A wrapper for the sparse_array<> type which is specialized for indices of type
        /// section_kind.
        template <typename T>
        class section_sparray {
        private:
            using index_type = std::underlying_type_t<section_kind>;
            static_assert (std::is_unsigned<index_type>::value,
                           "section-kind underlying type should be unsigned");

            /// Returns the maximum section-kind value.
            static constexpr auto max_section_kind () noexcept
                -> std::underlying_type_t<section_kind> {
                return static_cast<std::underlying_type_t<section_kind>> (section_kind::last);
            }

            /// Returns true if the value_type of Iterator is section_kind.
            template <typename Iterator>
            static constexpr bool is_section_kind_iterator () {
                return std::is_same<typename std::remove_cv<
                                        typename std::iterator_traits<Iterator>::value_type>::type,
                                    section_kind>::value;
            }

            using array_type = sparse_array<T, sparray_bitmap_t<max_section_kind ()>>;

        public:
            using value_type = typename array_type::value_type;
            using size_type = typename array_type::size_type;
            using iterator = typename array_type::iterator;
            using const_iterator = typename array_type::const_iterator;
            using reference = value_type &;
            using const_reference = value_type const &;

            /// Constructs a sparse array whose available indices are defined by the iterator range
            /// from [first,last) and whose corresponding values are default constructed.
            template <typename IndexIterator, typename = typename std::enable_if_t<
                                                  is_section_kind_iterator<IndexIterator> ()>>
            constexpr section_sparray (IndexIterator first, IndexIterator last);

            /// \name Element access
            ///@{
            constexpr value_type & operator[] (section_kind k) noexcept;
            constexpr value_type const & operator[] (section_kind k) const noexcept;

            /// Returns the value associated with the first section-kind index in the container.
            constexpr reference front () noexcept;
            /// Returns the value associated with the first section-kind index in the container.
            constexpr const_reference front () const noexcept;
            /// Returns the value associated with the last section-kind index in the container.
            constexpr reference back () noexcept;
            /// Returns the value associated with the last section-kind index in the container.
            constexpr const_reference back () const noexcept;
            ///@}


            /// Returns the number of bytes of storage that are required for an instance of
            /// section_sparray<T> where \p members is the number of available section-kinds.
            static constexpr std::size_t size_bytes (std::size_t members) noexcept;
            /// Returns the number of bytes of storage that are being used by this array instance.
            constexpr std::size_t size_bytes () const {
                return section_sparray<T>::size_bytes (this->size ());
            }

            /// \name Iterators
            ///@{

            /// Returns an iterator to the beginning of the container.
            iterator begin () { return sa_.begin (); }
            const_iterator begin () const { return sa_.begin (); }
            const_iterator cbegin () const { return sa_.cbegin (); }

            /// Returns an iterator to the end of the container.
            iterator end () { return sa_.end (); }
            const_iterator end () const { return sa_.end (); }
            const_iterator cend () const { return sa_.cend (); }

            ///@}


            /// \name Capacity
            ///@{
            constexpr bool empty () const noexcept { return sa_.empty (); }
            constexpr size_type size () const noexcept { return sa_.size (); }

            /// Returns true if the sparse array has an index \p pos.
            ///
            /// \param pos  The section kind to be tested.
            /// \returns True if the sparse array has an index for the \p pos section; false
            ///   otherwise.
            constexpr bool has_index (section_kind pos) const noexcept {
                return sa_.has_index (static_cast<index_type> (pos));
            }
            ///@}

            /// A container which can be used to access and iterate over the available index values
            /// in the sparse array.
            class indices {
            public:
                using iterator =
                    details::cast_iterator<section_kind,
                                           typename array_type::indices::const_iterator>;
                using const_iterator = iterator;

                constexpr explicit indices (typename array_type::indices const & i) noexcept
                        : i_{i} {}
                constexpr iterator begin () const noexcept { return iterator{i_.begin ()}; }
                constexpr iterator end () const noexcept { return iterator{i_.end ()}; }
                constexpr bool empty () const noexcept { return i_.empty (); }

                section_kind front () const noexcept {
                    return static_cast<section_kind> (i_.front ());
                }
                section_kind back () const noexcept {
                    return static_cast<section_kind> (i_.back ());
                }

            private:
                typename array_type::indices const i_;
            };

            indices get_indices () const noexcept { return indices{sa_.get_indices ()}; }

        private:
            array_type sa_;
        };

        // (ctor)
        // ~~~~~~
        template <typename T>
        template <typename IndexIterator, typename>
        constexpr section_sparray<T>::section_sparray (IndexIterator first, IndexIterator last)
                : sa_{details::cast_iterator<index_type, IndexIterator>{first},
                      details::cast_iterator<index_type, IndexIterator>{last}} {

            static_assert (
                std::numeric_limits<typename array_type::bitmap_type>::radix == 2,
                "expected numeric radix to be 2 (so that 'digits' is the number of bits)");
            static_assert (static_cast<index_type> (section_kind::last) <=
                               std::numeric_limits<typename array_type::bitmap_type>::digits,
                           "section_kind does not fit in the member sparse array");
        }

        // front
        // ~~~~~
        template <typename T>
        constexpr auto section_sparray<T>::front () noexcept -> reference {
            return sa_.front ();
        }
        template <typename T>
        constexpr auto section_sparray<T>::front () const noexcept -> const_reference {
            return sa_.front ();
        }
        // back
        // ~~~~
        template <typename T>
        constexpr auto section_sparray<T>::back () noexcept -> reference {
            return sa_.back ();
        }
        template <typename T>
        constexpr auto section_sparray<T>::back () const noexcept -> const_reference {
            return sa_.back ();
        }

        // operator[]
        // ~~~~~~~~~~
        template <typename T>
        constexpr auto section_sparray<T>::operator[] (section_kind k) noexcept -> value_type & {
            return sa_[static_cast<index_type> (k)];
        }
        template <typename T>
        constexpr auto section_sparray<T>::operator[] (section_kind k) const noexcept
            -> value_type const & {
            return sa_[static_cast<index_type> (k)];
        }

        // size bytes
        // ~~~~~~~~~~
        template <typename T>
        constexpr std::size_t section_sparray<T>::size_bytes (std::size_t const members) noexcept {
            return array_type::size_bytes (members);
        }

    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_SECTION_SPARRAY_HPP
