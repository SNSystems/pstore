//*          _        _                     _                *
//*  ___ ___| |_ _ __(_)_ __   __ _  __   _(_) _____      __ *
//* / __/ __| __| '__| | '_ \ / _` | \ \ / / |/ _ \ \ /\ / / *
//* \__ \__ \ |_| |  | | | | | (_| |  \ V /| |  __/\ V  V /  *
//* |___/___/\__|_|  |_|_| |_|\__, |   \_/ |_|\___| \_/\_/   *
//*                           |___/                          *
//===- include/pstore/sstring_view.hpp ------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
/// \brief Implements sstring_view, a class which is the same as std::string_view but holds a
/// std::shared_ptr<char> rather than a raw pointer.
///
/// This class is intended to improve the performance of the string set -- where it avoids the
/// construction of std::string instances -- and to enable string values from the database and
/// in-memory string values to be used interchangably.

#ifndef PSTORE_SSTRING_VIEW_HPP
#define PSTORE_SSTRING_VIEW_HPP

#include <cassert>
#include <iterator>
#include <memory>
#include <string>

#include "pstore_support/portab.hpp"

namespace pstore {

    class sstring_view;

    namespace sstring_view_details {

        inline std::size_t length (char const * s) {
            assert (s != nullptr);
            return std::char_traits<char>::length (s);
        }
        inline std::size_t length (std::string const & s) {
            return s.length ();
        }
        std::size_t length (sstring_view const & s);

        
        inline char const * data (char const * s) {
            assert (s != nullptr);
            return s;
        }
        inline char const * data (std::string const & s) {
            return s.data ();
        }
        char const * data (sstring_view const & s);

    } // sstring_view_details

    class sstring_view {
    public:
        using traits = std::char_traits<char>;
        using value_type = char;
        using pointer = char *;
        using const_pointer = char const *;
        using reference = char &;
        using const_reference = char const &;
        using const_iterator = char const *;
        using iterator = const_iterator; ///< Because sstring_view refers to a constant sequence,
                                         /// iterator and const_iterator are the same type.
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator = const_reverse_iterator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        static constexpr size_type npos = size_type (-1);

        // 7.3, sstring_view constructors and assignment operators
        sstring_view () noexcept = default;
        // sstring_view(sstring_view const &) noexcept = default;
        // sstring_view(sstring_view &&) noexcept = default;
        // sstring_view & operator=(sstring_view const &) noexcept = default;
        // sstring_view & operator=(sstring_view &&) noexcept = default;
        ////template<class Allocator>
        ////sstring_view (const basic_string<char, traits, Allocator> & str) noexcept;
        ////constexpr sstring_view (std::shared_ptr<char> str);
        sstring_view (std::shared_ptr<char> const & data, size_type size)
                : data_{data}
                , size_{size} {}

        // 7.4, sstring_view iterator support
        const_iterator begin () const noexcept {
            return cbegin ();
        }
        const_iterator end () const noexcept {
            return cend ();
        }
        const_iterator cbegin () const noexcept {
            return data_.get ();
        }
        const_iterator cend () const noexcept {
            return data_.get () + size_;
        }
        const_reverse_iterator rbegin () const noexcept;
        const_reverse_iterator rend () const noexcept;
        const_reverse_iterator crbegin () const noexcept;
        const_reverse_iterator crend () const noexcept;

        // 7.5, sstring_view capacity
        size_type size () const noexcept {
            return size_;
        }
        size_type length () const noexcept {
            return size_;
        }
        size_type max_size () const noexcept {
            return size_;
        }
        bool empty () const noexcept {
            return size_ == 0;
        }

        // 7.6, sstring_view element access
        const_reference operator[] (size_type pos) const {
            assert (pos < size_);
            return data_.get ()[pos];
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
            return data_.get ();
        }

        // 7.7, sstring_view modifiers
        void clear () noexcept {
            size_ = 0;
        }
        // constexpr void remove_prefix(size_type n);
        void remove_prefix (size_type n);
        void remove_suffix (size_type n);
        void swap (sstring_view & s) noexcept {
            std::swap (data_, s.data_);
            std::swap (size_, s.size_);
        }

        // 7.8, sstring_view string operations
        explicit operator std::string () const {
            return this->to_string ();
        }

        template <class Allocator = std::allocator<char>>
        std::basic_string<char, std::char_traits<char>, Allocator>
        to_string (Allocator const & a = Allocator ()) const {
            return {data_.get (), size_, a};
        }

        size_type copy (char * s, size_type n, size_type pos = 0) const;

        sstring_view substr (size_type pos = 0, size_type n = npos) const;

        //int compare (size_type pos1, size_type n1, sstring_view s) const;
        //int compare (size_type pos1, size_type n1, sstring_view s, size_type pos2,
        //             size_type n2) const;

        template <typename StringType>
        int compare (StringType const & s) const {
            auto const slen = sstring_view_details::length (s);
            size_type const common_len = std::min (size (), slen);
            int result = std::char_traits<char>::compare (data (), sstring_view_details::data (s),
                                                          common_len);
            if (result == 0) {
                result = size () == slen ? 0 : (size () < slen ? -1 : 1);
            }
            return result;
        }

        //int compare (size_type pos1, size_type n1, char const * s) const;
        //int compare (size_type pos1, size_type n1, char const * s, size_type n2) const;

#if 0
    size_type find(sstring_view s, size_type pos = 0) const noexcept;
    size_type find(char c, size_type pos = 0) const noexcept;
    size_type find(char const * s, size_type pos, size_type n) const;
    size_type find(char const * s, size_type pos = 0) const;

    size_type rfind(sstring_view s, size_type pos = npos) const noexcept;
    size_type rfind(char c, size_type pos = npos) const noexcept;
    size_type rfind(char const * s, size_type pos, size_type n) const;
    size_type rfind(char const * s, size_type pos = npos) const;

    size_type find_first_of(sstring_view s, size_type pos = 0) const noexcept;
    size_type find_first_of(char c, size_type pos = 0) const noexcept;
    size_type find_first_of(char const * s, size_type pos, size_type n) const;
    size_type find_first_of(char const * s, size_type pos = 0) const;

    size_type find_last_of(sstring_view s, size_type pos = npos) const noexcept;
    size_type find_last_of(char c, size_type pos = npos) const noexcept;
    size_type find_last_of(char const * s, size_type pos, size_type n) const;
    size_type find_last_of(char const * s, size_type pos = npos) const;

    size_type find_first_not_of(sstring_view s, size_type pos = 0) const noexcept;
    size_type find_first_not_of(char c, size_type pos = 0) const noexcept;
    size_type find_first_not_of(char const * s, size_type pos, size_type n) const;
    size_type find_first_not_of(char const * s, size_type pos = 0) const;

    size_type find_last_not_of(sstring_view s, size_type pos = npos) const noexcept;
    size_type find_last_not_of(char c, size_type pos = npos) const noexcept;
    size_type find_last_not_of(char const * s, size_type pos, size_type n) const;
    size_type find_last_not_of(char const * s, size_type pos = npos) const;
#endif

    private:
        std::shared_ptr<char> data_;
        size_type size_ = size_type{0};
    };

    namespace sstring_view_details {
        inline std::size_t length (sstring_view const & s) {
            return s.length ();
        }
        inline char const * data (sstring_view const & s) {
            return s.data ();
        }
    } // namespace sstring_view_details


    // [string.view.comparison]
    // operator==
    inline bool operator== (sstring_view const & lhs, sstring_view const & rhs) noexcept {
        if (lhs.size () != rhs.size ()) {
            return false;
        }
        return lhs.compare (rhs) == 0;
    }
    template <typename StringType>
    inline bool operator== (sstring_view const & lhs, StringType const & rhs) noexcept {
        if (sstring_view_details::length (lhs) != sstring_view_details::length (rhs)) {
            return false;
        }
        return lhs.compare (rhs) == 0;
    }
    template <typename StringType>
    inline bool operator== (StringType const & lhs, sstring_view const & rhs) noexcept {
        if (sstring_view_details::length (lhs) != sstring_view_details::length (rhs)) {
            return false;
        }
        return rhs.compare (lhs) == 0;
    }

    // operator!=
    inline bool operator!= (sstring_view lhs, sstring_view rhs) noexcept {
        return !operator== (lhs, rhs);
    }
    template <typename StringType>
    inline bool operator!= (sstring_view lhs, StringType const & rhs) noexcept {
        return !operator== (lhs, rhs);
    }
    template <typename StringType>
    inline bool operator!= (StringType const & lhs, sstring_view const & rhs) noexcept {
        return !operator== (lhs, rhs);
    }

    // operator>=
    inline bool operator>= (sstring_view const & lhs, sstring_view const & rhs) noexcept {
        return lhs.compare (rhs) >= 0;
    }
    template <typename StringType>
    inline bool operator>= (sstring_view const & lhs, StringType const & rhs) noexcept {
        return lhs.compare (rhs) >= 0;
    }
    template <typename StringType>
    inline bool operator>= (StringType const & lhs, sstring_view const & rhs) noexcept {
        return rhs.compare (lhs) <= 0;
    }

    // operator>
    inline bool operator> (sstring_view const & lhs, sstring_view const & rhs) noexcept {
        return lhs.compare (rhs) > 0;
    }
    template <typename StringType>
    inline bool operator> (sstring_view const & lhs, StringType const & rhs) noexcept {
        return lhs.compare (rhs) > 0;
    }
    template <typename StringType>
    inline bool operator> (StringType const & lhs, sstring_view const & rhs) noexcept {
        return rhs.compare (lhs) < 0;
    }

    // operator <=
    inline bool operator<= (sstring_view const & lhs, sstring_view const & rhs) noexcept {
        return lhs.compare (rhs) <= 0;
    }
    template <typename StringType>
    inline bool operator<= (sstring_view const & lhs, StringType const & rhs) noexcept {
        return lhs.compare (rhs) <= 0;
    }
    template <typename StringType>
    inline bool operator<= (StringType lhs, sstring_view const & rhs) noexcept {
        return rhs.compare (lhs) >= 0;
    }

    // operator <
    inline bool operator< (sstring_view const & lhs, sstring_view const & rhs) noexcept {
        return lhs.compare (rhs) < 0;
    }

    template <typename StringType>
    inline bool operator< (sstring_view const & lhs, StringType const & rhs) noexcept {
        return lhs.compare (rhs) < 0;
    }

    template <typename StringType>
    inline bool operator< (StringType const & lhs, sstring_view const & rhs) noexcept {
        return rhs.compare (lhs) > 0;
    }

} // namespace pstore

#endif // PSTORE_SSTRING_VIEW_HPP
// eof: include/pstore/sstring_view.hpp
