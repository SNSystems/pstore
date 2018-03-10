//*  _     _                        *
//* | |__ (_)_ __ ___   __ _ _ __   *
//* | '_ \| | '_ ` _ \ / _` | '_ \  *
//* | |_) | | | | | | | (_| | |_) | *
//* |_.__/|_|_| |_| |_|\__,_| .__/  *
//*                         |_|     *
//===- include/pstore/broker/bimap.hpp ------------------------------------===//
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
/// \file bimap.hpp

#ifndef PSTORE_BROKER_BIMAP_HPP
#define PSTORE_BROKER_BIMAP_HPP

#include <cassert>
#include <functional>
#include <map>

namespace pstore {
    namespace broker {

        /// A very simple bi-directional map in which either 'Left' or 'Right' type may be used as a
        /// key for
        /// lookup.
        template <typename Left, typename Right, typename LeftCompare = std::less<Left>,
                  typename RightCompare = std::less<Right>>
        class bimap {
            friend class iterator;
            using left_container = std::map<Left, Right, LeftCompare>;
            using right_container = std::map<Right, Left const *, RightCompare>;

        public:
            using ltype = Left;
            using rtype = Right;

            void set (Left const & a, Right const & b);

            Right const & getr (Left const & left);
            Left const & getl (Right const & right);

            bool presentl (Left const & a) const;
            bool presentr (Right const & b) const;

            ///@{
            /// Remove the element (if one exists) with a key equivalent to the supplied value.

            /// Searches the bimap for an element with a left key of type L2 (which must be
            /// implicitly
            /// convertable to type Left) and, if one exists, removes it.
            /// \param l2  Left key of the element to remove.
            template <typename L2>
            void erasel (L2 const & l2);

            /// Searches the bimap for an element with a right key of type R2 (which must be
            /// implicitly
            /// convertable to type Right) and, if one exists, removes it.
            /// \param r2  Right key of the element to remove.
            template <typename R2>
            void eraser (R2 const & r2);

            ///@}

            /// Returns the number of elements in the container.
            std::size_t size () const { return left_.size (); }

            template <typename MapIterator>
            class iterator : public std::iterator<std::forward_iterator_tag,
                                                  typename MapIterator::value_type::first_type> {
            public:
                using value_type = typename MapIterator::value_type::first_type;
                using value_reference_type = value_type const &;
                using value_pointer_type = value_type const *;

                iterator (MapIterator it)
                        : it_ (it) {}

                bool operator== (iterator const & other) const { return it_ == other.it_; }
                bool operator!= (iterator const & other) const { return it_ != other.it_; }

                /// Dereference operator
                /// @return the value of the element to which this iterator is currently pointing
                value_reference_type operator* () const { return it_->first; }
                value_pointer_type operator-> () const { return &it_->first; }
                /// Prefix increment
                iterator & operator++ () {
                    ++it_;
                    return *this;
                }
                /// Postfix increment operator (e.g., it++)
                iterator operator++ (int) {
                    iterator const old (*this);
                    it_++;
                    return old;
                }

            private:
                MapIterator it_;
            };

            using right_iterator = iterator<typename right_container::const_iterator>;
            right_iterator right_begin () const { return std::begin (right_); }
            right_iterator right_end () const { return std::end (right_); }

            using left_iterator = iterator<typename left_container::const_iterator>;
            left_iterator left_begin () const { return std::begin (left_); }
            left_iterator left_end () const { return std::end (left_); }

        private:
            left_container left_;
            right_container right_;
        };

        // set
        // ~~~
        template <typename L, typename R, typename Lcmp, typename Rcmp>
        void bimap<L, R, Lcmp, Rcmp>::set (L const & left, R const & right) {
// FIXME: set this in configure
#if PSTORE_BROKER_MAP_HAS_INSERT_OR_ASSIGN
            auto emplace_res = left_.insert_or_assign (left, right);
#else
            auto emplace_res = left_.emplace (left, right);
#endif

            auto & it = emplace_res.first;
            if (emplace_res.second) {
                // We inserted a new key-value pair. Update right_ to match.
                right_.emplace (it->second, &it->first);
            }
            assert (left_.size () == right_.size ());

#if !PSTORE_BROKER_MAP_HAS_INSERT_OR_ASSIGN
            it->second = right;
#endif
        }

        // get
        // ~~~
        template <typename L, typename R, typename Lcmp, typename Rcmp>
        R const & bimap<L, R, Lcmp, Rcmp>::getr (L const & left) {
            auto emplace_res = left_.emplace (left, R{});
            auto & it = emplace_res.first;
            if (emplace_res.second) {
                // We inserted a new key-value pair. Update r_ to match.
                right_.emplace (it->second, &it->first);
            }
            assert (left_.size () == right_.size ());
            return it->second;
        }
        template <typename L, typename R, typename Lcmp, typename Rcmp>
        L const & bimap<L, R, Lcmp, Rcmp>::getl (R const & right) {
            auto emplace_res = right_.emplace (right, nullptr);
            auto & it = emplace_res.first;
            if (emplace_res.second) {
                // We inserted a new key-value pair. Update l_ to match.
                assert (it->second == nullptr);
                auto r_a = left_.emplace (L{}, right);
                assert (r_a.second);
                // Point r_.second at the new L instance.
                it->second = &r_a.first->first;
            }
            assert (left_.size () == right_.size ());
            return *it->second;
        }


        // present
        // ~~~~~~~
        template <typename L, typename R, typename Lcmp, typename Rcmp>
        bool bimap<L, R, Lcmp, Rcmp>::presentl (L const & l) const {
            return left_.find (l) != left_.end ();
        }
        template <typename L, typename R, typename Lcmp, typename Rcmp>
        bool bimap<L, R, Lcmp, Rcmp>::presentr (R const & r) const {
            return right_.find (r) != right_.end ();
        }


        // erase
        // ~~~~~
        template <typename L, typename R, typename Lcmp, typename Rcmp>
        template <typename L2>
        void bimap<L, R, Lcmp, Rcmp>::erasel (L2 const & l2) {
            auto it = left_.find (l2);
            if (it != left_.end ()) {
                right_.erase (it->second);
                left_.erase (it->first);
            }
        }

        template <typename L, typename R, typename Lcmp, typename Rcmp>
        template <typename R2>
        void bimap<L, R, Lcmp, Rcmp>::eraser (R2 const & r2) {
            auto it = right_.find (r2);
            if (it != std::end (right_)) {
                auto & a = *it->second;
                left_.erase (a);
                right_.erase (it);
            }
        }

    } // namespace broker
} // namespace pstore

#endif // PSTORE_BROKER_BIMAP_HPP
// eof: include/pstore/broker/bimap.hpp
