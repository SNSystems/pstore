//*              _       _                        _             _              *
//*  _ __   ___ (_)_ __ | |_ ___  ___    __ _  __| | __ _ _ __ | |_ ___  _ __  *
//* | '_ \ / _ \| | '_ \| __/ _ \/ _ \  / _` |/ _` |/ _` | '_ \| __/ _ \| '__| *
//* | |_) | (_) | | | | | ||  __/  __/ | (_| | (_| | (_| | |_) | || (_) | |    *
//* | .__/ \___/|_|_| |_|\__\___|\___|  \__,_|\__,_|\__,_| .__/ \__\___/|_|    *
//* |_|                                                  |_|                   *
//===- include/pstore/support/pointee_adaptor.hpp -------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_SUPPORT_POINTEE_ADAPTOR_HPP
#define PSTORE_SUPPORT_POINTEE_ADAPTOR_HPP

#include <iterator>
#include <memory>

namespace pstore {

    /// An iterator adaptor which can be given an iterator which yields values of type pointer-to-T
    /// and, when dereferenced, will produce values of type T.
    template <typename Iterator>
    class pointee_adaptor {
    public:
        using value_type = typename std::pointer_traits<
            typename std::iterator_traits<Iterator>::value_type>::element_type;
        using difference_type = typename std::iterator_traits<Iterator>::difference_type;
        using pointer = value_type *;
        using reference = value_type &;
        using iterator_category = typename std::iterator_traits<Iterator>::iterator_category;

        pointee_adaptor () = default;
        explicit pointee_adaptor (Iterator it)
                : it_{it} {}
        pointee_adaptor (pointee_adaptor const & rhs) = default;
        pointee_adaptor & operator= (pointee_adaptor const & rhs) = default;

        bool operator== (pointee_adaptor const & rhs) const { return it_ == rhs.it_; }
        bool operator!= (pointee_adaptor const & rhs) const { return it_ != rhs.it_; }
        bool operator< (pointee_adaptor const & rhs) const { return it_ < rhs.it_; }
        bool operator> (pointee_adaptor const & rhs) const { return it_ > rhs.it_; }
        bool operator>= (pointee_adaptor const & rhs) const { return it_ >= rhs.it_; }
        bool operator<= (pointee_adaptor const & rhs) const { return it_ <= rhs.it_; }

        pointee_adaptor & operator++ () {
            ++it_;
            return *this;
        }
        pointee_adaptor operator++ (int) {
            auto const old = *this;
            ++it_;
            return old;
        }
        pointee_adaptor & operator-- () {
            --it_;
            return *this;
        }
        pointee_adaptor operator-- (int) {
            auto const old = *this;
            --it_;
            return old;
        }

        pointee_adaptor & operator+= (difference_type n) {
            it_ += n;
            return *this;
        }
        pointee_adaptor & operator-= (difference_type n) {
            it_ -= n;
            return *this;
        }

        difference_type operator- (pointee_adaptor const & rhs) const { return it_ - rhs.it_; }

        reference operator* () const { return **it_; }
        pointer operator-> () const { return &(**it_); }
        reference operator[] (difference_type n) const { return *(it_[n]); }

    private:
        Iterator it_;
    };

    template <typename Iterator>
    inline pointee_adaptor<Iterator>
    operator+ (pointee_adaptor<Iterator> a, typename pointee_adaptor<Iterator>::difference_type n) {
        return a += n;
    }

    template <typename Iterator>
    inline pointee_adaptor<Iterator>
    operator+ (typename pointee_adaptor<Iterator>::difference_type n, pointee_adaptor<Iterator> a) {
        return a += n;
    }

    template <typename Iterator>
    inline pointee_adaptor<Iterator>
    operator- (pointee_adaptor<Iterator> a, typename pointee_adaptor<Iterator>::difference_type n) {
        return a -= n;
    }

    template <typename Iterator>
    inline pointee_adaptor<Iterator> make_pointee_adaptor (Iterator it) {
        return pointee_adaptor<Iterator> (it);
    }

} // end namespace pstore

#endif // PSTORE_SUPPORT_POINTEE_ADAPTOR_HPP
