//===- include/pstore/broker/bimap.hpp --------------------*- mode: C++ -*-===//
//*  _     _                        *
//* | |__ (_)_ __ ___   __ _ _ __   *
//* | '_ \| | '_ ` _ \ / _` | '_ \  *
//* | |_) | | | | | | | (_| | |_) | *
//* |_.__/|_|_| |_| |_|\__,_| .__/  *
//*                         |_|     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file bimap.hpp

#ifndef PSTORE_BROKER_BIMAP_HPP
#define PSTORE_BROKER_BIMAP_HPP

#include <functional>
#include <map>

#include "pstore/support/assert.hpp"

namespace pstore {
    namespace broker {

        /// A very simple bi-directional map in which either 'Left' or 'Right' type may be used as a
        /// key for lookup.
        template <typename Left, typename Right, typename LeftCompare = std::less<Left>,
                  typename RightCompare = std::less<Right>>
        class bimap {
            friend class iterator;
            using left_container = std::map<Left, Right, LeftCompare>;
            using right_container = std::map<Right, Left const *, RightCompare>;

        public:
            using ltype = Left;
            using rtype = Right;

            void set (Left const & left, Right const & right);

            Right const & getr (Left const & left);
            Left const & getl (Right const & right);

            bool presentl (Left const & l) const;
            bool presentr (Right const & r) const;

            ///@{
            /// Remove the element (if one exists) with a key equivalent to the supplied value.

            /// Searches the bimap for an element with a left key of type L2 (which must be
            /// implicitly convertable to type Left) and, if one exists, removes it.
            /// \param l2  Left key of the element to remove.
            template <typename L2>
            void erasel (L2 const & l2);

            /// Searches the bimap for an element with a right key of type R2 (which must be
            /// implicitly convertable to type Right) and, if one exists, removes it.
            /// \param r2  Right key of the element to remove.
            template <typename R2>
            void eraser (R2 const & r2);

            ///@}

            /// Returns the number of elements in the container.
            std::size_t size () const { return left_.size (); }

            template <typename MapIterator>
            class iterator {
            public:
                using iterator_category = std::forward_iterator_tag;
                using value_type = typename MapIterator::value_type::first_type;
                using difference_type = std::ptrdiff_t;
                using pointer = value_type *;
                using reference = value_type &;

                explicit iterator (MapIterator it)
                        : it_ (it) {}

                bool operator== (iterator const & other) const { return it_ == other.it_; }
                bool operator!= (iterator const & other) const { return it_ != other.it_; }

                /// Dereference operator
                /// @return the value of the element to which this iterator is currently pointing
                value_type const & operator* () const { return it_->first; }
                value_type const * operator-> () const { return &it_->first; }
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
            right_iterator right_begin () const { return right_iterator{std::begin (right_)}; }
            right_iterator right_end () const { return right_iterator{std::end (right_)}; }

            using left_iterator = iterator<typename left_container::const_iterator>;
            left_iterator left_begin () const { return left_iterator{std::begin (left_)}; }
            left_iterator left_end () const { return left_iterator{std::end (left_)}; }

        private:
            left_container left_;
            right_container right_;
        };

        // set
        // ~~~
        template <typename L, typename R, typename Lcmp, typename Rcmp>
        void bimap<L, R, Lcmp, Rcmp>::set (L const & left, R const & right) {
#ifdef PSTORE_STD_MAP_HAS_INSERT_OR_ASSIGN
            auto emplace_res = left_.insert_or_assign (left, right);
#else
            auto emplace_res = left_.emplace (left, right);
#endif // PSTORE_STD_MAP_HAS_INSERT_OR_ASSIGN

            auto & it = emplace_res.first;
            if (emplace_res.second) {
                // We inserted a new key-value pair. Update right_ to match.
                right_.emplace (it->second, &it->first);
            }
            PSTORE_ASSERT (left_.size () == right_.size ());

#ifndef PSTORE_STD_MAP_HAS_INSERT_OR_ASSIGN
            it->second = right;
#endif // PSTORE_STD_MAP_HAS_INSERT_OR_ASSIGN
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
            PSTORE_ASSERT (left_.size () == right_.size ());
            return it->second;
        }
        template <typename L, typename R, typename Lcmp, typename Rcmp>
        L const & bimap<L, R, Lcmp, Rcmp>::getl (R const & right) {
            auto emplace_res = right_.emplace (right, nullptr);
            auto & it = emplace_res.first;
            if (emplace_res.second) {
                // We inserted a new key-value pair. Update l_ to match.
                PSTORE_ASSERT (it->second == nullptr);
                auto r_a = left_.emplace (L{}, right);
                PSTORE_ASSERT (r_a.second);
                // Point r_.second at the new L instance.
                it->second = &r_a.first->first;
            }
            PSTORE_ASSERT (left_.size () == right_.size ());
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
