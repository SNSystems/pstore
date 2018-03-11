//*  _                     _              _    *
//* | |__   __ _ _ __ ___ | |_   ___  ___| |_  *
//* | '_ \ / _` | '_ ` _ \| __| / __|/ _ \ __| *
//* | | | | (_| | | | | | | |_  \__ \  __/ |_  *
//* |_| |_|\__,_|_| |_| |_|\__| |___/\___|\__| *
//*                                            *
//===- include/pstore/core/hamt_set.hpp -----------------------------------===//
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
/// \file hamt_set.hpp

#ifndef PSTORE_HAMT_SET_HPP
#define PSTORE_HAMT_SET_HPP

// standard library
#include <iterator>

// pstore includes
#include "pstore/core/hamt_map.hpp"

namespace pstore {
    namespace index {

        template <typename KeyType, typename Hash, typename KeyEqual>
        class hamt_set final : public index_base {
            using index_pointer = pstore::index::details::index_pointer;

        private:
            class empty_class {};

            template <typename MapIterator>
            class set_iterator : public std::iterator<std::forward_iterator_tag,
                                                      typename std::add_const<KeyType>::type> {

                static_assert (std::is_same<typename MapIterator::value_type::first_type,
                                            typename std::add_const<KeyType>::type>::value,
                               "hamt_set key type does not match the iterator key type");

                using base = std::iterator<std::forward_iterator_tag,
                                           typename std::add_const<KeyType>::type>;
                using typename base::pointer;
                using typename base::reference;

            public:
                set_iterator (MapIterator const & it)
                        : it_ (it) {}

                bool operator== (set_iterator const & other) const { return it_ == other.it_; }

                bool operator!= (set_iterator const & other) const { return it_ != other.it_; }

                /// Dereference operator
                /// @return the value of the element to which this set_iterator is currently
                /// pointing
                reference operator* () const { return it_->first; }

                pointer operator-> () const { return &it_->first; }

                /// Prefix increment
                set_iterator & operator++ () {
                    ++it_;
                    return *this;
                }
                /// Postfix increment operator (e.g., it++)
                set_iterator operator++ (int) {
                    auto const old (*this);
                    it_++;
                    return old;
                }

                pstore::address get_address () const { return it_.get_address (); }

            private:
                MapIterator it_;
            };

        public:
            // types
            using key_type = KeyType;
            using value_type = key_type;
            using key_equal = KeyEqual;
            using hasher = Hash;
            using reference = value_type &;
            using const_reference = value_type const &;

            using const_iterator = set_iterator<
                typename hamt_map<value_type, empty_class, hasher, key_equal>::const_iterator>;
            using iterator = const_iterator;

            hamt_set (database & db, address ip = address::null (), hasher const & hash = hasher ())
                    : map_ (db, ip, hash) {}

            /// \name Iterators
            ///@{

            iterator begin () { return {map_.cbegin ()}; }
            iterator end () { return {map_.cend ()}; }

            const_iterator begin () const { return {map_.cbegin ()}; }
            const_iterator end () const { return {map_.cend ()}; }

            const_iterator cbegin () const { return {map_.cbegin ()}; }
            const_iterator cend () const { return {map_.cend ()}; }
            ///@}

            /// \name Capacity
            ///@{

            /// \brief Checks whether the container is empty.
            bool empty () const { return map_.empty (); }

            /// \brief Returns the number of elements in the container.
            std::size_t size () const { return map_.size (); }
            ///@}

            /// \brief Inserts an element into the container, if the container doesn't already
            /// contain an element with an equivalent key.
            ///
            /// \tparam OtherKeyType  A type whose serialized representation is compatible with
            /// KeyType.
            /// \param transaction  The transaction into which the new value element will be
            /// inserted.
            /// \param key  Element value to insert.
            /// \returns A pair consisting of an iterator to the inserted element (or to the element
            /// that prevented the insertion) and a bool value set to true if the insertion took
            /// place.
            template <typename OtherKeyType,
                      typename = typename std::enable_if<
                          serialize::is_compatible<KeyType, OtherKeyType>::value>::type>
            std::pair<iterator, bool> insert (transaction_base & transaction,
                                              OtherKeyType const & key) {
                auto it = map_.insert (transaction, std::make_pair (key, empty_class ()));
                return {{it.first}, it.second};
            }

            /// \brief Find the element with a specific key.
            /// Finds an element with key equivalent to \p key.
            ///
            /// \tparam OtherKeyType  A type whose serialized representation is compatible with
            /// KeyType.
            /// \param key  The key value of the element to search for.
            /// \return Iterator an an element with key equivalent to \p key. If no such element
            /// is found, the past-the-end iterator is returned.
            template <typename OtherKeyType,
                      typename = typename std::enable_if<
                          serialize::is_compatible<KeyType, OtherKeyType>::value>::type>
            const_iterator find (OtherKeyType const & key) const {
                return {map_.find (key)};
            }

            address flush (transaction_base & transaction) { return map_.flush (transaction); }

            /// \name Accessors
            /// Provide access to index internals.
            ///@{

            /// Read a leaf node from a store.
            value_type load_leaf_node (address const addr) const {
                return map_.load_leaf_node (addr).first;
            }

            database & db () { return map_.db (); }
            database const & db () const { return map_.db (); }

            index_pointer root () const { return map_.root (); }
            ///@}

        private:
            hamt_map<value_type, empty_class, hasher, key_equal> map_;
        };

    } // namespace index
} // namespace pstore
#endif // PSTORE_HAMT_SET_HPP
// eof: include/pstore/core/hamt_set.hpp