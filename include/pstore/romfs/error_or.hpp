//*                                         *
//*   ___ _ __ _ __ ___  _ __    ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__|  / _ \| '__| *
//* |  __/ |  | | | (_) | |    | (_) | |    *
//*  \___|_|  |_|  \___/|_|     \___/|_|    *
//*                                         *
//===- include/pstore/romfs/error_or.hpp ----------------------------------===//
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
#ifndef PSTORE_ROMFS_ERROR_OR_HPP
#define PSTORE_ROMFS_ERROR_OR_HPP

#include <new>
#include <system_error>

#include "pstore/support/inherit_const.hpp"
#include "pstore/support/max.hpp"
#include "pstore/support/portab.hpp"

namespace pstore {
    namespace romfs {

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
            explicit error_or (
                Other && t,
                typename std::enable_if<std::is_convertible<Other, T>::value>::type * = nullptr)
                    : state_ (value) {
                new (value_storage ()) T (std::forward<Other> (t));
            }

            error_or (error_or const & rhs) { copy_construct (rhs); }
            template <typename Other>
            error_or (
                error_or<Other> const & rhs,
                typename std::enable_if<std::is_convertible<Other, T>::value>::type * = nullptr) {
                copy_construct (rhs);
            }

            error_or (error_or && rhs) { move_construct (std::move (rhs)); }
            template <typename Other>
            error_or (
                error_or<Other> && rhs,
                typename std::enable_if<std::is_convertible<Other, T>::value>::type * = nullptr) {
                move_construct (std::move (rhs));
            }
            template <typename... Args>
            error_or (in_place_t, Args &&... args)
                    : state_ (value) {
                new (value_storage ()) T (std::forward<Args> (args)...);
            }

            ~error_or () noexcept { destroy (); }


            error_or * operator= (error_or const & rhs) {
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

        private:
            template <typename Other>
            void copy_construct (error_or<Other> const & rhs);
            template <typename Other>
            void move_construct (error_or<Other> && rhs);
            template <typename Other>
            void copy_assign (error_or<Other> const & rhs);
            void destroy () noexcept;


            std::error_code * error_storage () noexcept { return error_storage_impl (*this); }
            std::error_code const * error_storage () const noexcept {
                return error_storage_impl (*this);
            }

            T * value_storage () noexcept { return value_storage_impl (*this); }
            T const * value_storage () const noexcept { return value_storage_impl (*this); }

            template <typename ErrorOr,
                      typename ResultType = typename inherit_const<ErrorOr, std::error_code>::type>
            static ResultType * error_storage_impl (ErrorOr && e) noexcept {
                return reinterpret_cast<ResultType *> (&e.storage_);
            }
            template <typename ErrorOr,
                      typename ResultType = typename inherit_const<ErrorOr, T>::type>
            static ResultType * value_storage_impl (ErrorOr && e) noexcept {
                return reinterpret_cast<ResultType *> (&e.storage_);
            }


            using characteristics = pstore::characteristics<T, std::error_code>;
            typename std::aligned_storage<characteristics::size, characteristics::align>::type
                storage_;
            enum { error, value } state_;
        };

        template <typename T>
        template <typename Other>
        void error_or<T>::copy_construct (error_or<Other> const & rhs) {
            switch (rhs.state_) {
            case error:
                new (error_storage ()) std::error_code (rhs.get_error ());
                state_ = error;
                break;
            case value:
                new (value_storage ()) T (rhs.get_value ());
                state_ = value;
                break;
            }
        }

        template <typename T>
        template <typename Other>
        void error_or<T>::move_construct (error_or<Other> && rhs) {
            switch (rhs.state_) {
            case error:
                new (error_storage ()) std::error_code (std::move (rhs.get_error ()));
                state_ = error;
                break;
            case value:
                // note that we access the value storage directly here.
                new (value_storage ()) T (std::move (*rhs.value_storage ()));
                state_ = value;
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
                    // The standard error_code constructor is noexcept, so this is safe.
                    new (error_storage ()) std::error_code (std::move (e));
                    break;
                }
                state_ = error;
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
                    PSTORE_TRY {
                        new (value_storage ()) T (std::move (t));
                    } PSTORE_CATCH (..., {
                        // std::error_code ctors are noexcept, so this is safe.
                        new (error_storage ()) std::error_code (e);
                        state_ = error;
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
            return *this;
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

    } // end namespace romfs
} // end namespace pstore

#endif // PSTORE_ROMFS_ERROR_OR_HPP
