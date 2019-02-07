//*                                         *
//*   ___ _ __ _ __ ___  _ __    ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__|  / _ \| '__| *
//* |  __/ |  | | | (_) | |    | (_) | |    *
//*  \___|_|  |_|  \___/|_|     \___/|_|    *
//*                                         *
//===- include/pstore/support/error_or.hpp --------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_SUPPORT_ERROR_OR_HPP
#define PSTORE_SUPPORT_ERROR_OR_HPP

#include <new>
#include <system_error>

#include "pstore/support/inherit_const.hpp"
#include "pstore/support/max.hpp"
#include "pstore/support/portab.hpp"

namespace pstore {

    struct in_place_t {
        explicit in_place_t () = default;
    };
    constexpr in_place_t in_place{};

    template <typename T>
    class error_or {
    public:
        using value_type = T;

        explicit error_or (std::error_code ec)
                : state_ (error) {
            new (error_storage ()) std::error_code (std::move (ec));
        }
        template <typename Other>
        explicit error_or (Other && t,
                           typename std::enable_if<std::is_convertible<Other, T>::value>::type *
                               PSTORE_NULLABLE = nullptr)
                : state_ (value) {
            new (value_storage ()) T (std::forward<Other> (t));
        }
        error_or (error_or const & rhs)
                : state_{rhs.state_} {
            copy_construct (rhs);
        }
        error_or (error_or && rhs)
                : state_{rhs.state_} {
            move_construct (std::move (rhs));
        }
        template <typename... Args>
        error_or (in_place_t, Args &&... args)
                : state_ (value) {
            new (value_storage ()) T (std::forward<Args> (args)...);
        }

        ~error_or () noexcept { destroy (); }


        error_or & operator= (error_or const & rhs) {
            if (&rhs != this) {
                this->copy_assign (rhs);
            }
            return *this;
        }

        template <typename Other>
        error_or & operator= (error_or<Other> const & rhs) {
            this->copy_assign (rhs);
            return *this;
        }

        bool operator== (T const & rhs) const { return state_ == value && get_value () == rhs; }
        bool operator!= (T const & rhs) const { return !operator== (rhs); }

        bool operator== (error_or const & rhs) const {
            if (state_ != rhs.state_) {
                return false;
            }
            bool resl = false;
            switch (state_) {
            case error: resl = this->get_error () == rhs.get_error (); break;
            case value: resl = this->get_value () == rhs.get_value (); break;
            }
            return resl;
        }
        bool operator!= (error_or const & rhs) const { return !operator== (rhs); }

        bool has_error () const noexcept { return state_ == error; }
        bool has_value () const noexcept { return state_ == value; }

        std::error_code const & get_error () const noexcept { return *error_storage (); }
        T & get_value () noexcept { return *value_storage (); }
        T const & get_value () const noexcept { return *value_storage (); }

        T * PSTORE_NONNULL operator-> () noexcept { return value_storage (); }
        T const * PSTORE_NONNULL operator-> () const noexcept { return value_storage (); }

    private:
        void copy_construct (error_or const & rhs);
        void move_construct (error_or && rhs);
        template <typename Other>
        void copy_assign (error_or<Other> const & rhs);
        void destroy () noexcept;


        std::error_code * PSTORE_NONNULL error_storage () noexcept {
            return error_storage_impl (*this);
        }
        std::error_code const * PSTORE_NONNULL error_storage () const noexcept {
            return error_storage_impl (*this);
        }

        T * PSTORE_NONNULL value_storage () noexcept { return value_storage_impl (*this); }
        T const * PSTORE_NONNULL value_storage () const noexcept {
            return value_storage_impl (*this);
        }

        template <typename ErrorOr,
                  typename ResultType = typename inherit_const<ErrorOr, std::error_code>::type>
        static ResultType * PSTORE_NONNULL error_storage_impl (ErrorOr && e) noexcept {
            assert (e.state_ == error);
            return reinterpret_cast<ResultType *> (&e.storage_);
        }
        template <typename ErrorOr, typename ResultType = typename inherit_const<ErrorOr, T>::type>
        static ResultType * PSTORE_NONNULL value_storage_impl (ErrorOr && e) noexcept {
            assert (e.state_ == value);
            return reinterpret_cast<ResultType *> (&e.storage_);
        }


        using characteristics = pstore::characteristics<T, std::error_code>;
        typename std::aligned_storage<characteristics::size, characteristics::align>::type storage_;
        enum { error, value } state_;
    };

    template <typename T>
    void error_or<T>::copy_construct (error_or const & rhs) {
        switch (rhs.state_) {
        case error:
            state_ = error;
            new (error_storage ()) std::error_code (rhs.get_error ());
            break;
        case value:
            state_ = value;
            new (value_storage ()) T (rhs.get_value ());
            break;
        }
    }

    template <typename T>
    void error_or<T>::move_construct (error_or && rhs) {
        state_ = rhs.state_;
        switch (rhs.state_) {
        case error: new (error_storage ()) std::error_code (std::move (rhs.get_error ())); break;
        case value:
            // note that we access the value storage directly here.
            new (value_storage ()) T (std::move (*rhs.value_storage ()));
            break;
        }
    }

    template <typename T>
    template <typename Other>
    void error_or<T>::copy_assign (error_or<Other> const & rhs) {
        switch (rhs.state_) {
        case error: {
            std::error_code e = rhs.get_error ();
            switch (state_) {
            case error: std::swap (e, *this->error_storage ()); break;
            case value:
                this->destroy ();
                state_ = error;
                // The standard error_code constructor is noexcept, so this is safe.
                new (error_storage ()) std::error_code (std::move (e));
                break;
            }
            break;
        }
        case value: {
            Other t = rhs.get_value ();
            switch (state_) {
            case error: {
#if PSTORE_CPP_EXCEPTIONS
                std::error_code e = this->get_error ();
#endif
                this->destroy ();
                state_ = value;
                PSTORE_TRY { new (value_storage ()) T (std::move (t)); }
                PSTORE_CATCH (..., {
                    state_ = error;
                    // std::error_code ctors are noexcept, so this is safe.
                    new (error_storage ()) std::error_code (e);
                    throw;
                })
                break;
            }
            case value: std::swap (t, *this->value_storage ()); break;
            }
            state_ = value;
            break;
        }
        }
    }


    template <typename T>
    void error_or<T>::destroy () noexcept {
        switch (state_) {
        case value: get_value ().~T (); break;
        case error: {
            using type = std::error_code;
            get_error ().~type ();
            break;
        }
        }
    }


    template <typename T, typename Function>
    auto operator>>= (error_or<T> const & t, Function f) -> decltype (f (t.get_value ())) {
        if (t.has_value ()) {
            return f (t.get_value ());
        }
        return error_or<typename decltype (f (t.get_value ()))::value_type> (t.get_error ());
    }


} // end namespace pstore

#endif // PSTORE_SUPPORT_ERROR_OR_HPP
