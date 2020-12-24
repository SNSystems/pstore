//*                        _           *
//*  _ __ ___   __ _ _   _| |__   ___  *
//* | '_ ` _ \ / _` | | | | '_ \ / _ \ *
//* | | | | | | (_| | |_| | |_) |  __/ *
//* |_| |_| |_|\__,_|\__, |_.__/ \___| *
//*                  |___/             *
//===- include/pstore/support/maybe.hpp -----------------------------------===//
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
/// \file maybe.hpp
/// \brief An implementation of the Haskell Maybe type.
/// In Haskell, this simply looks like:
///    data Maybe a = Just a | Nothing
/// This is pretty much std::optional<> and this definition deliberately implements some of the
/// methods of that type, so we should switch to the standard type once we're able to migrate to
/// C++17.

#ifndef PSTORE_SUPPORT_MAYBE_HPP
#define PSTORE_SUPPORT_MAYBE_HPP

#include <new>
#include <stdexcept>

#include "pstore/adt/utility.hpp"
#include "pstore/support/assert.hpp"
#include "pstore/support/inherit_const.hpp"


namespace pstore {

    namespace details {

        template <typename T>
        struct remove_cvref {
            using type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
        };
        template <typename T>
        using remove_cvref_t = typename remove_cvref<T>::type;

    } // end namespace details

    template <typename T, typename = typename std::enable_if<!std::is_reference<T>::value>::type>
    class maybe {
    public:
        using value_type = T;

        static_assert (std::is_object<value_type>::value,
                       "Instantiation of maybe<> with a non-object type is undefined behavior.");
        static_assert (std::is_nothrow_destructible<value_type>::value,
                       "Instantiation of maybe<> with an object type that is not noexcept "
                       "destructible is undefined behavior.");

        /// Constructs an object that does not contain a value.
        constexpr maybe () noexcept = default;

        /// Constructs an optional object that contains a value, initialized with the expression
        /// std::forward<U>(value).
        template <typename U,
                  typename = typename std::enable_if<
                      std::is_constructible<T, U &&>::value &&
                      !std::is_same<typename details::remove_cvref_t<U>, maybe<T>>::value>::type>
        explicit maybe (U && value) noexcept (std::is_nothrow_move_constructible<T>::value &&
                                                  std::is_nothrow_copy_constructible<T>::value &&
                                              !std::is_convertible<U &&, T>::value) {

            new (&storage_) T (std::forward<U> (value));
            valid_ = true;
        }

        template <typename... Args>
        explicit maybe (in_place_t const, Args &&... args) {
            new (&storage_) T (std::forward<Args> (args)...);
            valid_ = true;
        }

        /// Copy constructor: If other contains a value, initializes the contained value with the
        /// expression *other. If other does not contain a value, constructs an object that does not
        /// contain a value.
        maybe (maybe const & other) noexcept (std::is_nothrow_copy_constructible<T>::value) {
            if (other.valid_) {
                new (&storage_) T (*other);
                valid_ = true;
            }
        }

        /// Move constructor: If other contains a value, initializes the contained value with the
        /// expression std::move(*other) and does not make other empty: a moved-from optional still
        /// contains a value, but the value itself is moved from. If other does not contain a value,
        /// constructs an object that does not contain a value.
        maybe (maybe && other) noexcept (std::is_nothrow_move_constructible<T>::value) {
            if (other.valid_) {
                new (&storage_) T (std::move (*other));
                valid_ = true;
            }
        }

        ~maybe () noexcept { this->reset (); }

        /// If *this contains a value, destroy it. *this does not contain a value after this call.
        void reset () noexcept {
            if (valid_) {
                // Set valid_ to false before calling the dtor.
                valid_ = false;
                reinterpret_cast<T const *> (&storage_)->~T ();
            }
        }

        maybe & operator= (maybe const & other) noexcept (
            std::is_nothrow_copy_assignable<T>::value &&
                std::is_nothrow_copy_constructible<T>::value) {
            if (&other != this) {
                if (!other.has_value ()) {
                    this->reset ();
                } else {
                    this->operator= (other.value ());
                }
            }
            return *this;
        }

        maybe &
        operator= (maybe && other) noexcept (std::is_nothrow_move_assignable<T>::value &&
                                                 std::is_nothrow_move_constructible<T>::value) {

            if (&other != this) {
                if (!other.has_value ()) {
                    this->reset ();
                } else {
                    this->operator= (std::forward<T> (other.value ()));
                }
            }
            return *this;
        }


        template <typename U,
                  typename = typename std::enable_if<
                      std::is_constructible<T, U &&>::value &&
                      !std::is_same<typename details::remove_cvref_t<U>, maybe<T>>::value>::type>
        maybe & operator= (U && other) noexcept (
            std::is_nothrow_copy_assignable<T>::value && std::is_nothrow_copy_constructible<
                T>::value && std::is_nothrow_move_assignable<T>::value &&
                std::is_nothrow_move_constructible<T>::value) {

            if (valid_) {
                T temp = std::forward<U> (other);
                std::swap (this->value (), temp);
            } else {
                new (&storage_) T (std::forward<U> (other));
                valid_ = true;
            }
            return *this;
        }

        /// Constructs the contained value in-place. If *this already contains a value before the
        /// call, the contained value is destroyed by calling its destructor.
        ///
        /// \p args... The arguments to pass to the constructor
        /// \returns A reference to the new contained value.
        template <typename... Args>
        T & emplace (Args &&... args) {
            if (valid_) {
                T temp (std::forward<Args> (args)...);
                std::swap (this->value (), temp);
            } else {
                new (&storage_) T (std::forward<Args> (args)...);
                valid_ = true;
            }
            return this->value ();
        }

        bool operator== (maybe const & other) const {
            return this->has_value () == other.has_value () &&
                   (!this->has_value () || this->value () == other.value ());
        }
        bool operator!= (maybe const & other) const { return !operator== (other); }

        /// accesses the contained value
        T const & operator* () const noexcept { return *(operator-> ()); }
        /// accesses the contained value
        T & operator* () noexcept { return *(operator-> ()); }
        /// accesses the contained value
        T const * operator-> () const noexcept {
            PSTORE_ASSERT (valid_);
            return reinterpret_cast<T const *> (&storage_);
        }
        /// accesses the contained value
        T * operator-> () noexcept {
            PSTORE_ASSERT (valid_);
            return reinterpret_cast<T *> (&storage_);
        }

        /// checks whether the object contains a value
        constexpr explicit operator bool () const noexcept { return valid_; }
        /// checks whether the object contains a value
        constexpr bool has_value () const noexcept { return valid_; }

        T const & value () const noexcept { return value_impl (*this); }
        T & value () noexcept { return value_impl (*this); }

        template <typename U>
        constexpr T value_or (U && default_value) const {
            return this->has_value () ? this->value () : default_value;
        }

    private:
        template <typename Maybe, typename ResultType = typename inherit_const<Maybe, T>::type>
        static ResultType & value_impl (Maybe && m) noexcept {
            PSTORE_ASSERT (m.has_value ());
            return *m;
        }

        bool valid_ = false;
        typename std::aligned_storage<sizeof (T), alignof (T)>::type storage_;
    };


    // just
    // ~~~~
    template <typename T>
    constexpr decltype (auto) just (T && value) {
        return maybe<typename details::remove_cvref_t<T>>{std::forward<T> (value)};
    }

    template <typename T, typename... Args>
    constexpr decltype (auto) just (in_place_t const, Args &&... args) {
        return maybe<typename details::remove_cvref_t<T>>{in_place, std::forward<Args> (args)...};
    }

    // nothing
    // ~~~~~~~
    template <typename T>
    constexpr decltype (auto) nothing () noexcept {
        return maybe<typename details::remove_cvref_t<T>>{};
    }

    /// The monadic "bind" operator for maybe<T>. If \p t is "nothing", then returns nothing where
    /// the type of the return is derived from the return type of \p f.  If \p t has a value then
    /// returns the result of calling \p f.
    ///
    /// \tparam T  The input type wrapped by a maybe<>.
    /// \tparam Function  A callable object whose signature is of the form `maybe<U> f(T t)`.
    template <typename T, typename Function>
    auto operator>>= (maybe<T> && t, Function f) -> decltype (f (*t)) {
        if (t) {
            return f (*t);
        }
        return maybe<typename decltype (f (*t))::value_type>{};
    }

} // namespace pstore

#endif // PSTORE_SUPPORT_MAYBE_HPP
