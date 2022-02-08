//===- include/pstore/adt/pointer_based_iterator.hpp ------*- mode: C++ -*-===//
//*              _       _              _                        _  *
//*  _ __   ___ (_)_ __ | |_ ___ _ __  | |__   __ _ ___  ___  __| | *
//* | '_ \ / _ \| | '_ \| __/ _ \ '__| | '_ \ / _` / __|/ _ \/ _` | *
//* | |_) | (_) | | | | | ||  __/ |    | |_) | (_| \__ \  __/ (_| | *
//* | .__/ \___/|_|_| |_|\__\___|_|    |_.__/ \__,_|___/\___|\__,_| *
//* |_|                                                             *
//*  _ _                 _              *
//* (_) |_ ___ _ __ __ _| |_ ___  _ __  *
//* | | __/ _ \ '__/ _` | __/ _ \| '__| *
//* | | ||  __/ | | (_| | || (_) | |    *
//* |_|\__\___|_|  \__,_|\__\___/|_|    *
//*                                     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file pointer_based_iterator.hpp
/// \brief Provides pointer_based_iterator<> an iterator wrapper for pointers.
///
/// Pointers to an array make perfectly good random access iterators. However there are a few of
/// minor niggles with their usage.
///
/// First, pointers sometimes take the value nullptr to indicate the end of a sequence. Consider
/// the POSIX readdir() API or a traditional singly-linked list where the last element of the list
/// has a 'next' pointer of nullptr.
///
/// Second, there’s no easy way to portably add debug-time checks to raw pointers. Having a class
/// allows us to sanity-check the value of the pointer relative to the container to which it points.
///
/// Third, our style guide (and clang-tidy) promotes the use of explicit '*' when declaring an
/// auto variable of pointer type. This is good, but if we're declarating the result type of a
/// standard library function that returns an iterator we would prefer to avoid hard-wiring the type
/// as a pointer and instead stick with an abstract 'iterator'.
///
/// The pointer_based_iterator<> class is intended to resolve both of these "problems" by providing
/// a random access iterator wrapper around a pointer.

#ifndef PSTORE_ADT_POINTER_BASED_ITERATOR_HPP
#define PSTORE_ADT_POINTER_BASED_ITERATOR_HPP

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace pstore {

    template <typename T>
    class pointer_based_iterator {
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;
        using iterator_category = std::random_access_iterator_tag;

        explicit constexpr pointer_based_iterator (std::nullptr_t) noexcept
                : pos_{nullptr} {}

        template <typename Other, typename = typename std::enable_if_t<
                                      std::is_const<T>::value && !std::is_const<Other>::value>>
        explicit constexpr pointer_based_iterator (Other const * const pos) noexcept
                : pos_{pos} {}

        template <typename Other, typename = typename std::enable_if_t<std::is_const<T>::value ==
                                                                       std::is_const<Other>::value>>
        explicit constexpr pointer_based_iterator (Other * const pos) noexcept
                : pos_{pos} {}

        template <typename Other>
        constexpr bool operator== (pointer_based_iterator<Other> const & other) const noexcept {
            return pos_ == &*other;
        }
        template <typename Other>
        constexpr bool operator!= (pointer_based_iterator<Other> const & other) const noexcept {
            return pos_ != &*other;
        }

        template <typename Other>
        constexpr pointer_based_iterator &
        operator= (pointer_based_iterator<Other> const & other) noexcept {
            pos_ = &*other;
            return *this;
        }

        constexpr value_type * operator-> () noexcept { return pos_; }
        constexpr value_type const * operator-> () const noexcept { return pos_; }
        constexpr value_type & operator* () noexcept { return *pos_; }
        constexpr value_type const & operator* () const noexcept { return *pos_; }

        constexpr value_type & operator[] (std::size_t const n) noexcept { return *(pos_ + n); }
        constexpr value_type const & operator[] (std::size_t const n) const noexcept {
            return *(pos_ + n);
        }

        pointer_based_iterator & operator++ () noexcept {
            ++pos_;
            return *this;
        }
        pointer_based_iterator operator++ (int) noexcept {
            auto const prev = *this;
            ++*this;
            return prev;
        }
        pointer_based_iterator & operator-- () noexcept {
            --pos_;
            return *this;
        }
        pointer_based_iterator operator-- (int) noexcept {
            auto const prev = *this;
            --*this;
            return prev;
        }

        pointer_based_iterator & operator+= (difference_type const n) noexcept {
            pos_ += n;
            return *this;
        }
        pointer_based_iterator & operator-= (difference_type const n) noexcept {
            pos_ -= n;
            return *this;
        }

        template <typename Other>
        constexpr bool operator< (pointer_based_iterator<Other> const & other) const noexcept {
            return pos_ < &*other;
        }
        template <typename Other>
        constexpr bool operator> (pointer_based_iterator<Other> const & other) const noexcept {
            return pos_ > &*other;
        }
        template <typename Other>
        constexpr bool operator<= (pointer_based_iterator<Other> const & other) const noexcept {
            return pos_ <= &*other;
        }
        template <typename Other>
        constexpr bool operator>= (pointer_based_iterator<Other> const & other) const noexcept {
            return pos_ >= &*other;
        }

    private:
        pointer pos_;
    };

    /// Move an iterator \p i forwards by distance \p n. \p n can be both positive or negative.
    /// \param i  The iterator to be moved.
    /// \param n  The distance by which iterator \p i should be moved.
    /// \returns  The new iterator.
    template <typename T>
    inline pointer_based_iterator<T>
    operator+ (pointer_based_iterator<T> const i,
               typename pointer_based_iterator<T>::difference_type const n) noexcept {
        auto temp = i;
        return temp += n;
    }
    /// Move an iterator \p i forwards by distance \p n. \p n can be both positive or negative.
    /// \param i  The iterator to be moved.
    /// \param n  The distance by which iterator \p i should be moved.
    /// \returns  The new iterator.
    template <typename T>
    inline pointer_based_iterator<T>
    operator+ (typename pointer_based_iterator<T>::difference_type const n,
               pointer_based_iterator<T> const i) noexcept {
        auto temp = i;
        return temp += n;
    }

    /// Move an iterator \p i backwards by distance \p n. \p n can be both positive or negative.
    /// \param i  The iterator to be moved.
    /// \param n  The distance by which iterator \p i should be moved.
    /// \returns  The new iterator.
    template <typename T>
    inline pointer_based_iterator<T>
    operator- (pointer_based_iterator<T> const i,
               typename pointer_based_iterator<T>::difference_type const n) noexcept {
        auto temp = i;
        return temp -= n;
    }

    template <typename T>
    inline typename pointer_based_iterator<T>::difference_type
    operator- (pointer_based_iterator<T> b, pointer_based_iterator<T> a) noexcept {
        return &*b - &*a;
    }

} // end namespace pstore

#endif // PSTORE_ADT_POINTER_BASED_ITERATOR_HPP
