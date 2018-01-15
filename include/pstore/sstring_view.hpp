//*          _        _                     _                *
//*  ___ ___| |_ _ __(_)_ __   __ _  __   _(_) _____      __ *
//* / __/ __| __| '__| | '_ \ / _` | \ \ / / |/ _ \ \ /\ / / *
//* \__ \__ \ |_| |  | | | | | (_| |  \ V /| |  __/\ V  V /  *
//* |___/___/\__|_|  |_|_| |_|\__, |   \_/ |_|\___| \_/\_/   *
//*                           |___/                          *
//===- include/pstore/sstring_view.hpp ------------------------------------===//
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
/// \file sstring_view.hpp
/// \brief Implements sstring_view, a class which is based on std::string_view but holds a
/// pointer which may be a std::shared_ptr<char> as well has a raw pointer.
///
/// This class is intended to improve the performance of the string set -- where it avoids the
/// construction of std::string instances -- and to enable string values from the database and
/// in-memory string values to be used interchangably.

#ifndef PSTORE_SSTRING_VIEW_HPP
#define PSTORE_SSTRING_VIEW_HPP

#include <array>
#include <cassert>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include "pstore/fnv.hpp"
#include "pstore/make_unique.hpp"
#include "pstore/varint.hpp"
#include "pstore_support/gsl.hpp"
#include "pstore_support/portab.hpp"

namespace pstore {

    //*     _       _             _            _ _       *
    //*  __| |_ _ _(_)_ _  __ _  | |_ _ _ __ _(_) |_ ___ *
    //* (_-<  _| '_| | ' \/ _` | |  _| '_/ _` | |  _(_-< *
    //* /__/\__|_| |_|_||_\__, |  \__|_| \__,_|_|\__/__/ *
    //*                   |___/                          *
    template <typename StringType>
    struct string_traits {};

    template <>
    struct string_traits<std::string> {
        static std::size_t length (std::string const & s) noexcept {
            return s.length ();
        }
        static char const * data (std::string const & s) noexcept {
            return s.data ();
        }
    };
    template <>
    struct string_traits<char const *> {
        static std::size_t length (char const * s) noexcept {
            return std::strlen (s);
        }
        static char const * data (char const * s) noexcept {
            return s;
        }
    };
    template <std::size_t Size>
    struct string_traits<char const[Size]> {
        static std::size_t length (char const * s) noexcept {
            return std::strlen (s);
        }
        static char const * data (char const * s) noexcept {
            return s;
        }
    };
    template <std::size_t Size>
    struct string_traits<char[Size]> {
        static std::size_t length (char const * s) noexcept {
            return std::strlen (s);
        }
        static char const * data (char const * s) noexcept {
            return s;
        }
    };

    //*            _     _             _            _ _       *
    //*  _ __  ___(_)_ _| |_ ___ _ _  | |_ _ _ __ _(_) |_ ___ *
    //* | '_ \/ _ \ | ' \  _/ -_) '_| |  _| '_/ _` | |  _(_-< *
    //* | .__/\___/_|_||_\__\___|_|    \__|_| \__,_|_|\__/__/ *
    //* |_|                                                   *
    template <typename PointerType>
    struct pointer_traits {
        static constexpr bool is_pointer = false;
    };
    template <>
    struct pointer_traits<char const *> {
        static constexpr bool is_pointer = true;
        using value_type = char const;
        static char const * as_raw (char const * p) noexcept {
            return p;
        }
    };

    namespace details {
        template <typename T>
        struct pointer_traits_helper {
            static constexpr bool is_pointer = true;
            using value_type = typename T::element_type;
            static_assert (std::is_same<value_type, char>::value ||
                               std::is_same<value_type, char const>::value,
                           "pointer element type must be char or char const");
            static char const * as_raw (T const & p) noexcept {
                return p.get ();
            }
        };
    } // namespace details

    template <>
    struct pointer_traits<std::shared_ptr<char const>>
        : details::pointer_traits_helper<std::shared_ptr<char const>> {};
    template <>
    struct pointer_traits<std::shared_ptr<char>>
        : details::pointer_traits_helper<std::shared_ptr<char>> {};
    template <>
    struct pointer_traits<std::unique_ptr<char const[]>>
        : details::pointer_traits_helper<std::unique_ptr<char const[]>> {};
    template <>
    struct pointer_traits<std::unique_ptr<char[]>>
        : details::pointer_traits_helper<std::unique_ptr<char[]>> {};

    //*        _       _                 _             *
    //*  _____| |_ _ _(_)_ _  __ _  __ _(_)_____ __ __ *
    //* (_-<_-<  _| '_| | ' \/ _` | \ V / / -_) V  V / *
    //* /__/__/\__|_| |_|_||_\__, |  \_/|_\___|\_/\_/  *
    //*                      |___/                     *
    template <typename PointerType>
    class sstring_view {
    public:
        static_assert (pointer_traits<PointerType>::is_pointer,
                       "PointerType is not a known pointer type!");

        using value_type = char const;
        using traits = std::char_traits<value_type>;
        using pointer = value_type *;
        using const_pointer = value_type const *;
        using reference = value_type &;
        using const_reference = value_type const &;
        using const_iterator = value_type const *;
        using iterator = const_iterator; ///< Because sstring_view refers to a constant sequence,
                                         /// iterator and const_iterator are the same type.
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator = const_reverse_iterator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        static constexpr size_type const npos = size_type (-1);

        // 7.3, sstring_view constructors and assignment operators
        sstring_view () noexcept
                : ptr_{nullptr}
                , size_{0U} {}
        sstring_view (PointerType ptr, size_type size) noexcept
                : ptr_{std::move (ptr)}
                , size_{size} {}
        sstring_view (sstring_view const &) = default;
        sstring_view (sstring_view &&) noexcept = default;
        ~sstring_view () = default;
        sstring_view & operator= (sstring_view const &) noexcept = default;
        sstring_view & operator= (sstring_view &&) noexcept = default;

        // 7.4, sstring_view iterator support
        const_iterator begin () const noexcept {
            return data ();
        }
        const_iterator end () const noexcept {
            return begin () + size_;
        }
        const_iterator cbegin () const noexcept {
            return begin ();
        }
        const_iterator cend () const noexcept {
            return end ();
        }
        const_reverse_iterator rbegin () const noexcept {
            return const_reverse_iterator (end ());
        }
        const_reverse_iterator rend () const noexcept {
            return const_reverse_iterator (begin ());
        }
        const_reverse_iterator crbegin () const noexcept {
            return const_reverse_iterator (end ());
        }
        const_reverse_iterator crend () const noexcept {
            return const_reverse_iterator (begin ());
        }

        // 7.5, sstring_view capacity
        size_type size () const noexcept {
            return size_;
        }
        size_type length () const noexcept {
            return size_;
        }
        size_type max_size () const noexcept {
            return std::numeric_limits<size_t>::max ();
        }
        bool empty () const noexcept {
            return size_ == 0;
        }

        // 7.6, sstring_view element access
        const_reference operator[] (size_type pos) const {
            assert (pos < size_);
            return (this->data ())[pos];
        }
        const_reference at (size_type pos) const {
#ifdef PSTORE_CPP_EXCEPTIONS
            if (pos >= size_) {
                throw std::out_of_range ("sstring_view access out of range");
            }
#endif
            return (*this)[pos];
        }
        const_reference front () const {
            assert (size_ > 0);
            return (*this)[0];
        }
        const_reference back () const {
            assert (size_ > 0);
            return (*this)[size_ - 1];
        }
        const_pointer data () const noexcept {
            return pointer_traits<PointerType>::as_raw (ptr_);
        }

        // 7.7, sstring_view modifiers
        void clear () noexcept {
            size_ = 0;
        }

        // void remove_prefix (size_type n);
        // void remove_suffix (size_type n);
        void swap (sstring_view & s) noexcept {
            std::swap (ptr_, s.ptr_);
            std::swap (size_, s.size_);
        }

        // 7.8, sstring_view string operations
        explicit operator std::string () const {
            return this->to_string ();
        }

        template <class Allocator = std::allocator<char>>
        std::basic_string<char, std::char_traits<char>, Allocator>
        to_string (Allocator const & a = Allocator ()) const {
            return {data (), size_, a};
        }

        // size_type copy (char * s, size_type n, size_type pos = 0) const;

        // sstring_view substr (size_type pos = 0, size_type n = npos) const;

        // int compare (size_type pos1, size_type n1, sstring_view s) const;
        // int compare (size_type pos1, size_type n1, sstring_view s, size_type pos2, size_type n2)
        // const;

        template <typename StringType>
        int compare (StringType const & s) const;

        // int compare (size_type pos1, size_type n1, char const * s) const;
        // int compare (size_type pos1, size_type n1, char const * s, size_type n2) const;

        /// Finds the first occurrence of `v` in this view, starting at position `pos`.
        // size_type find (sstring_view const & v, size_type pos = 0) const;
        // size_type find (char ch, size_type pos = 0) const;
        // size_type find (char const * s, size_type pos, size_type n) const;
        // size_type find (char const * s, size_type pos = 0) const;
        // size_type rfind(sstring_view s, size_type pos = npos) const noexcept;
        // size_type rfind(char c, size_type pos = npos) const noexcept;
        // size_type rfind(char const * s, size_type pos, size_type n) const;
        // size_type rfind(char const * s, size_type pos = npos) const;
        // size_type find_first_of(sstring_view s, size_type pos = 0) const noexcept;
        // size_type find_first_of(char c, size_type pos = 0) const noexcept;
        // size_type find_first_of(char const * s, size_type pos, size_type n) const;
        // size_type find_first_of(char const * s, size_type pos = 0) const;
        // size_type find_last_of(sstring_view s, size_type pos = npos) const noexcept;
        // size_type find_last_of(char c, size_type pos = npos) const noexcept;
        // size_type find_last_of(char const * s, size_type pos, size_type n) const;
        // size_type find_last_of(char const * s, size_type pos = npos) const;
        // size_type find_first_not_of(sstring_view s, size_type pos = 0) const noexcept;
        // size_type find_first_not_of(char c, size_type pos = 0) const noexcept;
        // size_type find_first_not_of(char const * s, size_type pos, size_type n) const;
        // size_type find_first_not_of(char const * s, size_type pos = 0) const;
        // size_type find_last_not_of(sstring_view s, size_type pos = npos) const noexcept;
        // size_type find_last_not_of(char c, size_type pos = npos) const noexcept;
        // size_type find_last_not_of(char const * s, size_type pos, size_type n) const;
        // size_type find_last_not_of(char const * s, size_type pos = npos) const;

    private:
        PointerType ptr_;
        size_type size_ = size_type{0};
    };

    template <typename PointerType>
    struct string_traits<sstring_view<PointerType>> {
        static std::size_t length (sstring_view<PointerType> const & s) noexcept {
            return s.length ();
        }
        static char const * data (sstring_view<PointerType> const & s) noexcept {
            return s.data ();
        }
    };

    template <typename PointerType>
    constexpr typename sstring_view<PointerType>::size_type sstring_view<PointerType>::npos;

    // compare
    // ~~~~~~~
    template <typename PointerType>
    template <typename StringType>
    int sstring_view<PointerType>::compare (StringType const & s) const {
        auto const slen = string_traits<StringType>::length (s);
        size_type const common_len = std::min (size (), slen);
        int result = traits::compare (data (), string_traits<StringType>::data (s), common_len);
        if (result == 0) {
            result = (size () == slen) ? 0 : (size () < slen ? -1 : 1);
        }
        return result;
    }

    // operator==
    // ~~~~~~~~~~
    template <typename PointerType1, typename PointerType2>
    inline bool operator== (sstring_view<PointerType1> const & lhs,
                            sstring_view<PointerType2> const & rhs) noexcept {
        if (lhs.size () != rhs.size ()) {
            return false;
        }
        return lhs.compare (rhs) == 0;
    }
    template <typename PointerType, typename StringType>
    inline bool operator== (sstring_view<PointerType> const & lhs,
                            StringType const & rhs) noexcept {
        if (string_traits<sstring_view<PointerType>>::length (lhs) !=
            string_traits<StringType>::length (rhs)) {
            return false;
        }
        return lhs.compare (rhs) == 0;
    }
    template <typename StringType, typename PointerType>
    inline bool operator== (StringType const & lhs,
                            sstring_view<PointerType> const & rhs) noexcept {
        if (string_traits<StringType>::length (lhs) !=
            string_traits<sstring_view<PointerType>>::length (rhs)) {
            return false;
        }
        return rhs.compare (lhs) == 0;
    }

    // operator!=
    // ~~~~~~~~~~
    template <typename PointerType1, typename PointerType2>
    inline bool operator!= (sstring_view<PointerType1> const & lhs,
                            sstring_view<PointerType2> const & rhs) noexcept {
        return !operator== (lhs, rhs);
    }
    template <typename PointerType, typename StringType>
    inline bool operator!= (sstring_view<PointerType> const & lhs,
                            StringType const & rhs) noexcept {
        return !operator== (lhs, rhs);
    }
    template <typename StringType, typename PointerType>
    inline bool operator!= (StringType const & lhs,
                            sstring_view<PointerType> const & rhs) noexcept {
        return !operator== (lhs, rhs);
    }

    // operator>=
    // ~~~~~~~~~~
    template <typename PointerType1, typename PointerType2>
    inline bool operator>= (sstring_view<PointerType1> const & lhs,
                            sstring_view<PointerType2> const & rhs) noexcept {
        return lhs.compare (rhs) >= 0;
    }
    template <typename PointerType, typename StringType>
    inline bool operator>= (sstring_view<PointerType> const & lhs,
                            StringType const & rhs) noexcept {
        return lhs.compare (rhs) >= 0;
    }
    template <typename StringType, typename PointerType>
    inline bool operator>= (StringType const & lhs,
                            sstring_view<PointerType> const & rhs) noexcept {
        return rhs.compare (lhs) <= 0;
    }

    // operator>
    // ~~~~~~~~~
    template <typename PointerType1, typename PointerType2>
    inline bool operator> (sstring_view<PointerType1> const & lhs,
                           sstring_view<PointerType2> const & rhs) noexcept {
        return lhs.compare (rhs) > 0;
    }
    template <typename PointerType, typename StringType>
    inline bool operator> (sstring_view<PointerType> const & lhs, StringType const & rhs) noexcept {
        return lhs.compare (rhs) > 0;
    }
    template <typename StringType, typename PointerType>
    inline bool operator> (StringType const & lhs, sstring_view<PointerType> const & rhs) noexcept {
        return rhs.compare (lhs) < 0;
    }

    // operator<=
    // ~~~~~~~~~~
    template <typename PointerType1, typename PointerType2>
    inline bool operator<= (sstring_view<PointerType1> const & lhs,
                            sstring_view<PointerType2> const & rhs) noexcept {
        return lhs.compare (rhs) <= 0;
    }
    template <typename PointerType, typename StringType>
    inline bool operator<= (sstring_view<PointerType> const & lhs,
                            StringType const & rhs) noexcept {
        return lhs.compare (rhs) <= 0;
    }
    template <typename StringType, typename PointerType>
    inline bool operator<= (StringType const & lhs,
                            sstring_view<PointerType> const & rhs) noexcept {
        return rhs.compare (lhs) >= 0;
    }

    // operator<
    // ~~~~~~~~~
    template <typename PointerType1, typename PointerType2>
    inline bool operator< (sstring_view<PointerType1> const & lhs,
                           sstring_view<PointerType2> const & rhs) noexcept {
        return lhs.compare (rhs) < 0;
    }
    template <typename PointerType, typename StringType>
    inline bool operator< (sstring_view<PointerType> const & lhs, StringType const & rhs) noexcept {
        return lhs.compare (rhs) < 0;
    }
    template <typename StringType, typename PointerType>
    inline bool operator< (StringType const & lhs, sstring_view<PointerType> const & rhs) noexcept {
        return rhs.compare (lhs) > 0;
    }

    // operator <<
    // ~~~~~~~~~~~
    template <typename PointerType>
    inline std::ostream & operator<< (std::ostream & os, sstring_view<PointerType> const & str) {
        return os.write (str.data (), str.length ());
    }


    //*             _               _       _                 _             *
    //*  _ __  __ _| |_____   _____| |_ _ _(_)_ _  __ _  __ _(_)_____ __ __ *
    //* | '  \/ _` | / / -_) (_-<_-<  _| '_| | ' \/ _` | \ V / / -_) V  V / *
    //* |_|_|_\__,_|_\_\___| /__/__/\__|_| |_|_||_\__, |  \_/|_\___|\_/\_/  *
    //*                                           |___/                     *
    template <typename ValueType>
    inline sstring_view<std::shared_ptr<ValueType>>
    make_sstring_view (std::shared_ptr<ValueType> const & ptr, std::size_t length) {
        return {ptr, length};
    }
    template <typename ValueType>
    inline sstring_view<std::unique_ptr<ValueType>>
    make_sstring_view (std::unique_ptr<ValueType> ptr, std::size_t length) {
        return {std::move (ptr), length};
    }
    inline sstring_view<char const *> make_sstring_view (char const * ptr, std::size_t length) {
        return {ptr, length};
    }

} // namespace pstore

namespace std {

    template <typename StringType>
    struct equal_to<::pstore::sstring_view<StringType>> {
        template <typename S1, typename S2>
        bool operator() (S1 const & x, S2 const & y) const {
            return x == y;
        }
    };

    template <typename StringType>
    struct hash <::pstore::sstring_view <StringType>> {
        size_t operator() (::pstore::sstring_view<StringType> const & str) const {
            return static_cast <size_t> (::pstore::fnv_64a_buf (str.data (), str.length ()));
        }
    };

} // namespace std

#endif // PSTORE_SSTRING_VIEW_HPP
// eof: include/pstore/sstring_view.hpp
