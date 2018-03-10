//*                        _           *
//*  _ __ ___   __ _ _   _| |__   ___  *
//* | '_ ` _ \ / _` | | | | '_ \ / _ \ *
//* | | | | | | (_| | |_| | |_) |  __/ *
//* |_| |_| |_|\__,_|\__, |_.__/ \___| *
//*                  |___/             *
//===- include/pstore/support/maybe.hpp -----------------------------------===//
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
#ifndef PSTORE_CMD_UTIL_CL_MAYBE_HPP
#define PSTORE_CMD_UTIL_CL_MAYBE_HPP

#include <cassert>
#include <new>
#include <stdexcept>
#include <type_traits>

#include "pstore/support/portab.hpp"

namespace pstore {

    template <typename T>
    class maybe {
    public:
        using value_type = T;

        static maybe<T> just (T const & value) { return maybe (value); }
        static maybe<T> nothing () { return maybe (); }

        maybe () {}
        maybe (T const & value) {
            ::new (&storage_) T (value);
            valid_ = true;
        }
        maybe (T && value) {
            ::new (&storage_) T (std::move (value));
            valid_ = true;
        }
        maybe (maybe const & other) {
            if (other) {
                ::new (&storage_) T (*other);
                valid_ = true;
            }
        }
        maybe (maybe && other) {
            if (other) {
                ::new (&storage_) T (std::move (*other));
                valid_ = true;
            }
        }

        ~maybe () { this->reset (); }

        void reset () {
            if (valid_) {
                // Set valid_ to false before calling the dtor in case it throws. We need to avoid
                // the possibility of calling it a second time from our dtor.
                valid_ = false;
                reinterpret_cast<T const *> (&storage_)->~T ();
            }
        }
        maybe & operator= (maybe const & rhs);
        maybe & operator= (T const & rhs);
        maybe &
        operator= (maybe && other) noexcept (std::is_nothrow_move_assignable<T>::value &&
                                                 std::is_nothrow_move_constructible<T>::value);
        maybe & operator= (T && other);

        T const & operator* () const noexcept { return *(operator-> ()); }
        T & operator* () noexcept { return *(operator-> ()); }
        T const * operator-> () const noexcept {
            assert (valid_);
            return reinterpret_cast<T const *> (&storage_);
        }
        T * operator-> () noexcept {
            assert (valid_);
            return reinterpret_cast<T *> (&storage_);
        }

        constexpr operator bool () const noexcept { return valid_; }
        constexpr bool has_value () const noexcept { return valid_; }

        T const & value () const { return const_cast<maybe<T> *> (this)->value (); }
        T & value ();

        template <typename U>
        constexpr T value_or (U && default_value) const {
            return valid_ ? this->value () : default_value;
        }

    private:
        bool valid_ = false;
        typename std::aligned_storage<sizeof (T), alignof (T)>::type storage_;
    };

    // operator=
    // ~~~~~~~~~
    template <typename T>
    maybe<T> & maybe<T>::operator= (T const & rhs) {
        if (this->has_value ()) {
            T temp = rhs;
            std::swap (this->value (), temp);
        } else {
            ::new (&storage_) T (rhs);
            valid_ = true;
        }
        return *this;
    }

    template <typename T>
    maybe<T> & maybe<T>::operator= (T && other) {
        if (this->valid_) {
            *(*this) = std::forward<T> (other);
        } else {
            ::new (&storage_) T (std::forward<T> (other));
            this->valid_ = true;
        }
        return *this;
    }

    template <typename T>
    maybe<T> & maybe<T>::operator= (maybe<T> const & other) {
        if (!other.valid_) {
            this->reset ();
        } else {
            this->operator= (*other);
        }
        return *this;
    }

    template <typename T>
    maybe<T> & maybe<T>::operator= (maybe<T> && rhs) noexcept (
        std::is_nothrow_move_assignable<T>::value && std::is_nothrow_move_constructible<T>::value) {
        if (!rhs.valid_) {
            this->reset ();
        } else {
            this->operator= (std::forward<T> (*rhs));
        }
        return *this;
    }


    // value
    // ~~~~~
    template <typename T>
    inline T & maybe<T>::value () {
#if PSTORE_CPP_EXCEPTIONS
        if (!has_value ()) {
            throw std::runtime_error ("no value");
        }
#else
        assert (has_value ());
#endif
        return *(*this);
    }

    // just
    // ~~~~
    template <typename T>
    inline maybe<T> just (T const value) {
        return {value};
    }

    // nothing
    // ~~~~~~~
    template <typename T>
    inline maybe<T> nothing () {
        return maybe<T>::nothing ();
    }

} // namespace pstore

#endif // PSTORE_CMD_UTIL_CL_MAYBE_HPP
// eof: include/pstore/support/maybe.hpp
