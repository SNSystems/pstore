//===- include/pstore/adt/error_or.hpp --------------------*- mode: C++ -*-===//
//*                                         *
//*   ___ _ __ _ __ ___  _ __    ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__|  / _ \| '__| *
//* |  __/ |  | | | (_) | |    | (_) | |    *
//*  \___|_|  |_|  \___/|_|     \___/|_|    *
//*                                         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file error_or.hpp
/// \brief error_or<T> holds either an instance of T or a std:error.

#ifndef PSTORE_ADT_ERROR_OR_HPP
#define PSTORE_ADT_ERROR_OR_HPP

#include <new>
#include <system_error>
#include <tuple>

#include "pstore/adt/utility.hpp"
#include "pstore/support/assert.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/inherit_const.hpp"

namespace pstore {

    template <typename Error>
    struct is_error : std::integral_constant<bool, std::is_error_code_enum<Error>::value ||
                                                       std::is_error_condition_enum<Error>::value> {
    };

    template <typename T>
    class error_or {
        template <typename Other>
        friend class error_or;

        using wrapper = std::reference_wrapper<typename std::remove_reference<T>::type>;
        using storage_type =
            typename std::conditional<std::is_reference<T>::value, wrapper, T>::type;

    public:
        using value_type = T;
        using reference = T &;
        using const_reference = typename std::remove_reference<T>::type const &;
        using pointer = gsl::not_null<typename std::remove_reference<T>::type *>;
        using const_pointer = gsl::not_null<typename std::remove_reference<T>::type const *>;

        // ****
        // construction
        // ****

        template <typename ErrorCode,
                  typename = typename std::enable_if<is_error<ErrorCode>::value>::type>
        explicit error_or (ErrorCode const erc) {
            new (get_error_storage ()) std::error_code (make_error_code (erc));
        }

        explicit error_or (std::error_code const erc) {
            new (get_error_storage ()) std::error_code (erc);
        }

        template <typename Other,
                  typename = typename std::enable_if<std::is_convertible<Other, T>::value>::type>
        explicit error_or (Other && other)
                : has_error_{false} {
            new (get_storage ()) storage_type (std::forward<Other> (other));
        }

        template <typename... Args>
        explicit error_or (in_place_t const inp, Args &&... args)
                : has_error_{false} {
            (void) inp;
            new (get_storage ()) storage_type (std::forward<Args> (args)...);
        }

        error_or (error_or const & rhs) { copy_construct (rhs); }

        template <typename Other,
                  typename = typename std::enable_if<std::is_convertible<Other, T>::value>::type>
        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        error_or (error_or<Other> const & rhs) {
            copy_construct (rhs);
        }

        error_or (error_or && rhs) noexcept { move_construct (std::move (rhs)); }

        template <typename Other,
                  typename = typename std::enable_if<std::is_convertible<Other, T>::value>::type>
        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        error_or (error_or<Other> && rhs) noexcept {
            move_construct (std::move (rhs));
        }

        ~error_or ();


        // ****
        // assignment
        // ****
        template <typename ErrorCode,
                  typename = typename std::enable_if<is_error<ErrorCode>::value>::type>
        error_or & operator= (ErrorCode rhs) {
            operator=(error_or<T>{rhs});
            return *this;
        }
        error_or & operator= (std::error_code const & rhs) {
            operator= (error_or<T> (rhs));
            return *this;
        }
        error_or & operator= (error_or const & rhs) {
            copy_assign (rhs);
            return *this;
        }
        error_or & operator= (error_or && rhs) {
            move_assign (std::move (rhs));
            return *this;
        }


        // ****
        // comparison (operator== and operator!=)
        // ****
        bool operator== (std::error_code const rhs) const { return get_error () == rhs; }
        bool operator== (error_or const & rhs);
        bool operator== (T const & rhs) const {
            return static_cast<bool> (*this) && this->get () == rhs;
        }
        template <typename Error>
        typename std::enable_if<is_error<Error>::value, bool>::type operator== (Error rhs) const {
            return get_error () == rhs;
        }

        bool operator!= (T const & rhs) const { return !operator== (rhs); }
        bool operator!= (std::error_code const rhs) const { return !operator== (rhs); }
        bool operator!= (error_or const & rhs) { return !operator== (rhs); }
        template <typename Error>
        typename std::enable_if<is_error<Error>::value, bool>::type operator!= (Error rhs) const {
            return !operator== (rhs);
        }


        // ****
        // access
        // ****

        /// Return true if a value is held, otherwise false.
        explicit operator bool () const noexcept { return !has_error_; }

        std::error_code get_error () const noexcept;

        reference get () noexcept { return *get_storage (); }
        const_reference get () const noexcept { return *get_storage (); }
        pointer operator-> () noexcept { return to_pointer (get_storage ()); }
        const_pointer operator-> () const noexcept { return to_pointer (get_storage ()); }
        reference operator* () noexcept { return *get_storage (); }
        const_reference operator* () const noexcept { return *get_storage (); }

    private:
        template <typename Other>
        void copy_construct (error_or<Other> const & rhs);

        template <typename T1>
        static constexpr bool same_object (T1 const & lhs, T1 const & rhs) noexcept {
            return &lhs == &rhs;
        }

        template <typename T1, typename T2>
        static constexpr bool same_object (T1 const & lhs, T2 const & rhs) noexcept {
            (void) lhs;
            (void) rhs;
            return false;
        }

        template <typename Other>
        error_or & copy_assign (error_or<Other> const & rhs);

        template <typename Other>
        void move_construct (error_or<Other> && rhs) noexcept;

        template <typename Other>
        error_or & move_assign (error_or<Other> && rhs);

        pointer to_pointer (wrapper * PSTORE_NONNULL val) noexcept { return &val->get (); }
        pointer to_pointer (pointer val) noexcept { return val; }
        const_pointer to_pointer (const_pointer val) const noexcept { return val; }
        const_pointer to_pointer (wrapper const * PSTORE_NONNULL val) const noexcept {
            return &val->get ();
        }

        template <typename ErrorOr,
                  typename ResultType = typename inherit_const<ErrorOr, storage_type>::type>
        static ResultType * PSTORE_NONNULL value_storage_impl (ErrorOr && e) noexcept {
            PSTORE_ASSERT (!e.has_error_);
            return reinterpret_cast<ResultType *> (&e.storage_);
        }

        storage_type * PSTORE_NONNULL get_storage () noexcept { return value_storage_impl (*this); }
        storage_type const * PSTORE_NONNULL get_storage () const noexcept {
            return value_storage_impl (*this);
        }

        template <typename ErrorOr,
                  typename ResultType = typename inherit_const<ErrorOr, std::error_code>::type>
        static ResultType * PSTORE_NONNULL error_storage_impl (ErrorOr && e) noexcept {
            PSTORE_ASSERT (e.has_error_);
            return reinterpret_cast<ResultType *> (&e.error_storage_);
        }
        std::error_code * PSTORE_NONNULL get_error_storage () noexcept;
        std::error_code const * PSTORE_NONNULL get_error_storage () const noexcept;

        union {
            typename std::aligned_storage<sizeof (storage_type), alignof (storage_type)>::type
                storage_;
            std::aligned_storage<sizeof (std::error_code), alignof (std::error_code)>::type
                error_storage_;
        };
        bool has_error_ = true;
    };


    // dtor
    // ~~~~
    template <typename T>
    error_or<T>::~error_or () {
        if (!has_error_) {
            get_storage ()->~storage_type ();
        } else {
            using error_code = std::error_code;
            get_error_storage ()->~error_code ();
        }
    }

    // copy_construct
    // ~~~~~~~~~~~~~~
    template <typename T>
    template <typename Other>
    void error_or<T>::copy_construct (error_or<Other> const & rhs) {
        has_error_ = rhs.has_error_;
        if (has_error_) {
            new (get_error_storage ()) std::error_code (rhs.get_error ());
        } else {
            new (get_storage ()) storage_type (*rhs.get_storage ());
        }
    }

    // copy_assign
    // ~~~~~~~~~~~
    template <typename T>
    template <typename Other>
    auto error_or<T>::copy_assign (error_or<Other> const & rhs) -> error_or & {
        if (same_object (*this, rhs)) {
            return *this;
        }

        if (has_error_ && rhs.has_error_) {
            *get_error_storage () = *rhs.get_error_storage ();
        } else if (!has_error_ && !rhs.has_error_) {
            *get_storage () = *rhs.get_storage ();
        } else {
            this->~error_or ();
            new (this) error_or (rhs);
        }
        return *this;
    }

    // move_construct
    // ~~~~~~~~~~~~~~
    template <typename T>
    template <typename Other>
    void error_or<T>::move_construct (error_or<Other> && rhs) noexcept {
        has_error_ = rhs.has_error_;
        if (has_error_) {
            new (get_error_storage ()) std::error_code (rhs.get_error ());
        } else {
            new (get_storage ()) storage_type (std::move (*rhs.get_storage ()));
        }
    }

    // move_assign
    // ~~~~~~~~~~~
    template <typename T>
    template <typename Other>
    auto error_or<T>::move_assign (error_or<Other> && rhs) -> error_or & {
        if (same_object (*this, rhs)) {
            return *this;
        }

        if (has_error_ && rhs.has_error_) {
            *get_error_storage () = std::move (*rhs.get_error_storage ());
        } else if (!has_error_ && !rhs.has_error_) {
            *get_storage () = std::move (*rhs.get_storage ());
        } else {
            this->~error_or ();
            new (this) error_or (std::move (rhs));
        }
        return *this;
    }

    // operator==
    // ~~~~~~~~~~
    template <typename T>
    bool error_or<T>::operator== (error_or const & rhs) {
        if (has_error_ != rhs.has_error_) {
            return false;
        }
        if (has_error_) {
            return this->operator== (rhs.get_error ());
        }
        return this->operator== (rhs.get ());
    }

    // get_error
    // ~~~~~~~~~
    template <typename T>
    std::error_code error_or<T>::get_error () const noexcept {
        return has_error_ ? *get_error_storage () : std::error_code{};
    }

    // get_error_storage
    // ~~~~~~~~~~~~~~~~~
    template <typename T>
    inline std::error_code * PSTORE_NONNULL error_or<T>::get_error_storage () noexcept {
        return error_storage_impl (*this);
    }
    template <typename T>
    inline std::error_code const * PSTORE_NONNULL error_or<T>::get_error_storage () const noexcept {
        return error_storage_impl (*this);
    }


    // operator>>= (bind)
    // ~~~~~~~~~~~
    /// The monadic "bind" operator for error_or<T>. If \p t has an error then then returns the
    /// error where the return type is derived from the return type of \p f.  If \p t has a value
    /// then returns the result of calling \p f.
    ///
    /// \tparam T  The input type wrapped by a error_or<>.
    /// \tparam Function  A callable object whose signature is of the form `error_or<U> f(T t)`.
    template <typename T, typename Function>
    auto operator>>= (error_or<T> && t, Function f) -> decltype ((f (t.get ()))) {
        if (t) {
            return f (*t);
        }
        return error_or<typename decltype (f (t.get ()))::value_type> (t.get_error ());
    }


    template <typename... Args>
    using error_or_n = error_or<std::tuple<Args...>>;


    template <std::size_t I, class... Types>
    constexpr typename std::tuple_element<I, std::tuple<Types...>>::type &
    get (pstore::error_or_n<Types...> & eon) noexcept {
        return std::get<I> (*eon);
    }

    template <std::size_t I, class... Types>
    constexpr typename std::tuple_element<I, std::tuple<Types...>>::type &&
    get (pstore::error_or_n<Types...> && eon) noexcept {
        return std::get<I> (*eon);
    }

    template <std::size_t I, class... Types>
    constexpr typename std::tuple_element<I, std::tuple<Types...>>::type const &
    get (pstore::error_or_n<Types...> const & eon) noexcept {
        return std::get<I> (*eon);
    }

    namespace details {

        template <std::size_t N>
        struct applier {
            template <typename Function, typename Tuple, typename... Args>
            static auto apply (Function && f, Tuple && t, Args &&... a)
                -> decltype (applier<N - 1U>::apply (std::forward<Function> (f),
                                                     std::forward<Tuple> (t),
                                                     std::get<N - 1U> (std::forward<Tuple> (t)),
                                                     std::forward<Args> (a)...)) {

                return applier<N - 1U>::apply (std::forward<Function> (f), std::forward<Tuple> (t),
                                               std::get<N - 1U> (std::forward<Tuple> (t)),
                                               std::forward<Args> (a)...);
            }
        };

        template <>
        struct applier<std::size_t{0}> {
            template <typename Function, typename Tuple, typename... Args>
            static auto apply (Function && f, Tuple && t, Args &&... a)
                -> decltype (std::forward<Function> (f) (std::forward<Args> (a)...)) {
                (void) t;
                return std::forward<Function> (f) (std::forward<Args> (a)...);
            }
        };

        template <typename Function, typename Tuple>
        inline auto apply (Function && f, Tuple && t)
            -> decltype (applier<std::tuple_size<typename std::decay<Tuple>::type>::value>::apply (
                std::forward<Function> (f), std::forward<Tuple> (t))) {

            return applier<std::tuple_size<typename std::decay<Tuple>::type>::value>::apply (
                std::forward<Function> (f), std::forward<Tuple> (t));
        }

    } // end namespace details


    template <typename Function, typename... Args>
    auto operator>>= (error_or_n<Args...> const & t, Function && f)
        -> decltype (details::apply (f, t.get ())) {

        if (t) {
            return details::apply (std::forward<Function> (f), t.get ());
        }
        return decltype (details::apply (f, t.get ())){t.get_error ()};
    }

} // end namespace pstore

#endif // PSTORE_ADT_ERROR_OR_HPP
