//*  _                     _                            *
//* | |__   __ _ _ __ ___ | |_   _ __ ___   __ _ _ __   *
//* | '_ \ / _` | '_ ` _ \| __| | '_ ` _ \ / _` | '_ \  *
//* | | | | (_| | | | | | | |_  | | | | | | (_| | |_) | *
//* |_| |_|\__,_|_| |_| |_|\__| |_| |_| |_|\__,_| .__/  *
//*                                             |_|     *
//===- include/pstore/hamt_map.hpp ----------------------------------------===//
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
/// \file hamt_map.hpp

#ifndef PSTORE_HAMT_MAP_HPP
#define PSTORE_HAMT_MAP_HPP (1)

#include <iterator>
#include <type_traits>

#include "pstore/database.hpp"
#include "pstore/hamt_map_fwd.hpp"
#include "pstore/hamt_map_types.hpp"
#include "pstore/serialize/standard_types.hpp"
#include "pstore/transaction.hpp"

namespace pstore {

    class transaction_base;

    namespace index {
        using ::pstore::gsl::not_null;

        inline index_base::~index_base () {}
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4521)
#endif
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        class hamt_map final : public index_base {
            using hash_type = pstore::index::details::hash_type;
            using index_pointer = pstore::index::details::index_pointer;
            using internal_node = pstore::index::details::internal_node;
            using linear_node = pstore::index::details::linear_node;
            using parent_stack = pstore::index::details::parent_stack;

            /// A helper class which provides a member constant `value`` which is equal to true if
            /// types K and V have a serialized representation which is compatible with KeyType and
            /// ValueType respectively. Otherwise `value` is false.
            template <typename K, typename V>
            struct pair_types_compatible
                : std::integral_constant<bool, serialize::is_compatible<KeyType, K>::value &&
                                                   serialize::is_compatible<ValueType, V>::value> {
            };

        public:
            using key_equal = KeyEqual;
            using key_type = KeyType;
            using mapped_type = ValueType;
            using value_type = std::pair<KeyType const, ValueType>;

            /// Inner class that describes a const_iterator and 'regular' iterator.
            template <bool IsConstIterator = true>
            class iterator_base : public std::iterator<std::forward_iterator_tag, value_type> {
                // Make iterator_base<true> a friend class of iterator_base<false> so the copy
                // constructor can access the private member variables.
                friend class iterator_base<true>;
                friend class hamt_map;
                using parent_stack =
                    typename hamt_map<KeyType, ValueType, Hash, KeyEqual>::parent_stack;
                using index_pointer =
                    typename hamt_map<KeyType, ValueType, Hash, KeyEqual>::index_pointer;

            public:
                /**
                 * For const_iterator: define value_reference_type to be a 'value_type const &'
                 * For iterator:       define value_reference_type to be a 'value_type &'
                 */
                using value_reference_type =
                    typename std::conditional<IsConstIterator, value_type const &,
                                              value_type &>::type;

                /**
                 * For const_iterator:   define value_pointer_type to be a   'value_type const *'
                 * For regular iterator: define value_pointer_type to be a   'value_type *'
                 */
                using value_pointer_type =
                    typename std::conditional<IsConstIterator, value_type const *,
                                              value_type *>::type;

                iterator_base (parent_stack && parents, hamt_map const * idx)
                        : visited_parents_ (std::move (parents))
                        , index_ (idx) {}

                iterator_base (iterator_base const & other) noexcept
                        : visited_parents_ (other.visited_parents_)
                        , index_ (other.index_) {
                    pos_.reset ();
                }

                /**
                 * Copy constructor. Allows for implicit conversion from a regular iterator to a
                 * const_iterator
                 */
                template <bool Enable = IsConstIterator,
                          typename = typename std::enable_if<Enable>::type>
                iterator_base (iterator_base<false> const & other) noexcept
                        : visited_parents_ (other.visited_parents_)
                        , index_ (other.index_) {
                    pos_.reset ();
                }

                iterator_base & operator= (iterator_base const & rhs) noexcept {
                    visited_parents_ = rhs.visited_parents_;
                    index_ = rhs.index_;
                    pos_.reset ();
                    return *this;
                }

                bool operator== (iterator_base const & other) const {
                    return index_ == other.index_ && visited_parents_ == other.visited_parents_;
                }

                bool operator!= (iterator_base const & other) const {
                    return !operator== (other);
                }

                /// Dereference operator
                /// \return The value of the element to which this iterator is currently pointing.
                value_reference_type operator* () const {
                    if (pos_ == nullptr) {
                        pos_ = std::make_unique<value_type> (
                            index_->load_leaf_node (this->get_address ()));
                    }
                    return *pos_;
                }

                value_pointer_type operator-> () const {
                    return &operator* ();
                }

                /// Prefix increment
                iterator_base & operator++ ();

                /// Postfix increment operator (e.g., it++)
                iterator_base operator++ (int) {
                    iterator_base const old (*this);
                    ++(*this);
                    return old;
                }

                /// Returns the pstore address of the serialized value_type instance to which the
                /// iterator is currently pointing.
                address get_address () const {
                    assert (visited_parents_.size () >= 1);
                    details::parent_type const & parent = visited_parents_.top ();
                    assert (parent.node.is_leaf () && parent.position == details::not_found);
                    return parent.node.addr;
                }

            private:
                void increment_internal_node ();

                /// Walks the iterator's position to point to the deepest, left-most leaf of the
                /// the current node. The iterator must be positing to an internal node when this
                /// method is called.
                void move_to_left_most_child (index_pointer node);

                unsigned get_shift_bits () const {
                    assert (visited_parents_.size () >= 1);
                    return static_cast<unsigned> ((visited_parents_.size () - 1) *
                                                  details::hash_index_bits);
                }

                parent_stack visited_parents_;
                hamt_map const * index_;
                mutable std::unique_ptr<value_type> pos_;
            };

            using iterator = iterator_base<false>;
            using const_iterator = iterator_base<true>;

            /// An associative container that contains key-value pairs with unique keys.
            ///
            /// \param db A database which will contain the result of the hamt_map.
            /// \param ip An index root address.
            /// \param hash A hash function that generates the hash value from the key value.
            hamt_map (database & db, address ip = address::null (), Hash const & hash = Hash ())
                    : db_ (db)
                    , root_ (ip)
                    , hash_ (hash)
                    , equal_ (KeyEqual ()) {
                if (root_.is_heap ()) {
                    raise (pstore::error_code::index_corrupt);
                }
            }

            ~hamt_map () {
                this->clear ();
            }

            /// \name Iterators
            ///@{

            /// Returns an iterator to the beginning of the container
            iterator begin () {
                return make_begin_iterator<iterator> ();
            }
            const_iterator begin () const {
                return make_begin_iterator<const_iterator> ();
            }
            const_iterator cbegin () const {
                return make_begin_iterator<const_iterator> ();
            }

            /// Returns an iterator to the end of the container
            iterator end () {
                return iterator (parent_stack (), this);
            }
            const_iterator end () const {
                return const_iterator (parent_stack (), this);
            }
            const_iterator cend () const {
                return const_iterator (parent_stack (), this);
            }
            ///@}

            /// \name Capacity
            ///@{

            /// Checks whether the container is empty
            bool empty () const {
                return root_.is_empty ();
            }

            /// Returns the number of elements
            std::size_t size () const {
                auto s = std::distance (cbegin (), cend ());
                assert (s >= 0);
                return static_cast<std::size_t> (s);
            }
            ///@}

            /// \name Modifiers
            ///@{

            /// Inserts an element into the hamt_map if the hamt_map doesn't already contain an
            /// element with an equivalent key. If insertion occurs, all iterators are invalidated.
            ///
            /// \tparam OtherKeyType  A type whose serialized representation is compatible with
            /// KeyType.
            /// \tparam OtherValueType  A type whose serialized representation is compatible with
            /// ValueType.
            /// \param transaction The transaction to which the new key-value pair will be appended.
            /// \param value  The key-value pair to be inserted.
            /// \result The bool component is true if the insertion took place and false if the
            /// assignment took place. The iterator component points at the exiting or new element.
            template <typename OtherKeyType, typename OtherValueType,
                      typename = typename std::enable_if<
                          pair_types_compatible<OtherKeyType, OtherValueType>::value>::type>
            auto insert (transaction_base & transaction,
                         std::pair<OtherKeyType, OtherValueType> const & value)
                -> std::pair<iterator, bool>;

            /// If a key equivalent to \p value first already exists in the container, assigns
            /// \p value second to the mapped type. If the key does not exist, inserts the new value
            /// as if by insert(). If insertion occurs, all iterators are invalidated.
            ///
            /// \tparam OtherKeyType  A type whose serialized representation is compatible with
            /// KeyType.
            /// \tparam OtherValueType  A type whose serialized representation is compatible with
            /// ValueType.
            /// \param transaction  The transaction to which new data will be appended.
            /// \param value  The key-value pair to be inserted or updated.
            /// \result The bool component is true if the insertion took place and false if the
            /// assignment took place. The iterator component points at the element inserted or
            /// updated.
            template <typename OtherKeyType, typename OtherValueType,
                      typename = typename std::enable_if<
                          pair_types_compatible<OtherKeyType, OtherValueType>::value>::type>
            auto insert_or_assign (transaction_base & transaction,
                                   std::pair<OtherKeyType, OtherValueType> const & value)
                -> std::pair<iterator, bool>;

            /// If a key equivalent to \p key already exists in the container, assigns
            /// \p value to the mapped type. If the key does not exist, inserts the new value as
            /// if by insert(). If insertion occurs, all iterators are invalidated.
            ///
            /// \tparam OtherKeyType  A type whose serialized representation is compatible with
            /// KeyType.
            /// \tparam OtherValueType  A type whose serialized representation is compatible with
            /// ValueType.
            /// \param transaction  The transaction to which new data will be appended.
            /// \param key  The key the used both to look up and to insert if not found.
            /// \param value  The value that will be associated with 'key' after the call.
            /// \result The bool component is true if the insertion took place and false if the
            /// assignment took place. The iterator component points at the element inserted or
            /// updated.
            template <typename OtherKeyType, typename OtherValueType,
                      typename = typename std::enable_if<
                          pair_types_compatible<OtherKeyType, OtherValueType>::value>::type>
            auto insert_or_assign (transaction_base & transaction, OtherKeyType const & key,
                                   OtherValueType const & value) -> std::pair<iterator, bool>;
            ///@}

            /// Finds an element with key equivalent to \p key.
            ///
            /// \tparam OtherKeyType  A type whose serialized representation is compatible with
            /// KeyType.
            /// \param  key key value of the element to be found.
            /// \return Iterator to an element with key equivalent to key. If not such element is
            ///         found, past-the end iterator it returned.
            template <typename OtherKeyType,
                      typename = typename std::enable_if<
                          serialize::is_compatible<OtherKeyType, KeyType>::value>::type>
            const_iterator find (OtherKeyType const & key) const;

            /// Write an hamt_map into a store when transaction::commit is called.
            address flush (transaction_base & transaction);

            /// \name Accessors
            /// Provide access to index internals.
            ///@{

            /// Read a leaf node from a store.
            value_type load_leaf_node (address const addr) const;

            /// Returns the database instance to which the index belongs.
            database & db () {
                return db_;
            }
            database const & db () const {
                return db_;
            }

            /// Returns the index root pointer.
            index_pointer root () const {
                return root_;
            }
            ///@}

        private:
            /// Stores a key/value data pair.
            template <typename OtherValueType>
            address store_leaf_node (transaction_base & transaction, OtherValueType const & v,
                                     not_null<parent_stack *> parents);

            /// If the \p node is a heap internal node, clear its children and itself.
            void clear (index_pointer node, unsigned shifts);

            /// Clear the hamt_map when transaction::rollback() function is called.
            void clear () {
                if (root_.is_heap ()) {
                    this->clear (root_, 0 /*shifts*/);
                    root_.internal = nullptr;
                }
            }

            /// Read a key from a store.
            key_type get_key (address const addr) const;

            /// Called when the trie's top-level loop has descended as far as a leaf node. We need
            /// to convert that to an internal node.
            template <typename OtherValueType>
            auto insert_into_leaf (transaction_base & transaction,
                                   index_pointer const & existing_leaf,
                                   OtherValueType const & new_leaf, hash_type existing_hash,
                                   hash_type hash, unsigned shifts,
                                   not_null<parent_stack *> parents) -> index_pointer;

            /// Inserts a key-value pair into an internal node, potentially traversing to deeper
            /// nodes in the tree.
            ///
            /// \param transaction  The transaction to which new data will be appended.
            /// \param node  A heap or in-store reference to an existing internal node.
            /// \param value The key/value pair to be inserted.
            /// \param hash  The key hash.
            /// \param shifts  The number of bits by which the hash value is shifted to reach the
            /// current tree level.
            /// \param parents  A stack containing references to the nodes visited during the tree
            /// traversal (and the positions within each of those nodes). This is later used to
            /// build an iterator instance.
            /// \param is_upsert  True if this is an "upsert" (insert or update) operation, false
            /// otherwise.
            /// \result  A pair consisting of a reference to the internal node (which will be equal
            /// to \p node if the nothing was modified by the insert operation) and a bool denoting
            /// whether the key was already present.
            template <typename OtherValueType>
            auto insert_into_internal (transaction_base & transaction, index_pointer node,
                                       OtherValueType const & value, hash_type hash,
                                       unsigned shifts, not_null<parent_stack *> parents,
                                       bool is_upsert) -> std::pair<index_pointer, bool>;

            template <typename OtherValueType>
            auto insert_into_linear (transaction_base & transaction, index_pointer const node,
                                     OtherValueType const & value, not_null<parent_stack *> parents,
                                     bool is_upsert) -> std::pair<index_pointer, bool>;

            /// Insert a new key/value pair into a existing node, which could be a leaf node, an
            /// internal store node or an internal heap node.
            template <typename OtherValueType>
            auto insert_node (transaction_base & transaction, index_pointer const node,
                              OtherValueType const & value, hash_type hash, unsigned shifts,
                              not_null<parent_stack *> parents, bool is_upsert)
                -> std::pair<index_pointer, bool>;

            template <typename IteratorType>
            IteratorType make_begin_iterator () const;

            /// Insert or insert_or_assign a node into a hamt_map.
            /// \tparam OtherValueType  A type whose serialization is compatible with value_type.
            template <typename OtherValueType>
            std::pair<iterator, bool> insert_or_upsert (transaction_base & transaction,
                                                        OtherValueType const & value,
                                                        bool is_upsert);

            /// Frees memory consumed by a heap-allocated tree node.
            ///
            /// \param node  The tree node to be deleted.
            /// \param shifts  The number of bits by which the hash value is shifted to reach the
            /// current tree level. This is used to determine whether the reference is to an
            /// internal or linear node.
            void delete_node (index_pointer node, unsigned shifts);

            database & db_;
            index_pointer root_;
            Hash hash_;
            KeyEqual equal_;
        };
#ifdef _WIN32
#pragma warning(pop)
#endif

        //*  _ _                 _               _                     *
        //* (_) |_ ___ _ __ __ _| |_ ___  _ __  | |__   __ _ ___  ___  *
        //* | | __/ _ \ '__/ _` | __/ _ \| '__| | '_ \ / _` / __|/ _ \ *
        //* | | ||  __/ | | (_| | || (_) | |    | |_) | (_| \__ \  __/ *
        //* |_|\__\___|_|  \__,_|\__\___/|_|    |_.__/ \__,_|___/\___| *
        //*                                                            *
        // Prefix increment
        // ~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <bool IsConstIterator>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::iterator_base<IsConstIterator>::
        operator++ () -> iterator_base & {
            pos_.reset ();
            assert (!visited_parents_.empty ());
            this->increment_internal_node ();
            return *this;
        }


        // increment_internal_node
        // ~~~~~~~~~~~~~~~~~~~~~~~
        /// Move the iterator points to the next child.
        /// If the last child of this node is reached, so we need to:
        /// 1. Move to its parent
        /// 2. Figure out which of the parent's children we've just completed
        /// 3. Was that the last of the parent's children? If so, got to step 1.
        /// 4. If this next node is an internal node, find the its deepest, left-most
        ///    child.
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <bool IsConstIterator>
        void hamt_map<KeyType, ValueType, Hash,
                      KeyEqual>::iterator_base<IsConstIterator>::increment_internal_node () {

            visited_parents_.pop ();

            if (!visited_parents_.empty ()) {
                std::shared_ptr<void const> node;
                internal_node const * internal = nullptr;
                linear_node const * linear = nullptr;

                details::parent_type parent = visited_parents_.top ();
                unsigned const shifts = this->get_shift_bits ();
                bool const is_internal_node = details::depth_is_internal_node (shifts);
                std::size_t size = 0;
                if (is_internal_node) {
                    std::tie (node, internal) =
                        internal_node::get_node (index_->db (), parent.node);
                    assert (internal != nullptr);
                    size = internal->size ();
                } else {
                    std::tie (node, linear) = linear_node::get_node (index_->db (), parent.node);
                    assert (linear != nullptr);
                    size = linear->size ();
                }

                assert (!parent.node.is_leaf () && parent.position < size);
                ++parent.position;

                if (parent.position >= size) {
                    this->increment_internal_node ();
                } else {
                    // Update the parent.
                    visited_parents_.top ().position = parent.position;

                    // Visit the child.
                    if (is_internal_node) {
                        index_pointer child = (*internal)[parent.position];
                        if (child.is_internal ()) {
                            this->move_to_left_most_child (child);
                        } else {
                            visited_parents_.push ({child});
                        }
                    } else {
                        visited_parents_.push ({(*linear)[parent.position]});
                    }
                }
            }
        }

        // move_to_left_most_child
        // ~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <bool IsConstIterator>
        void hamt_map<KeyType, ValueType, Hash, KeyEqual>::iterator_base<
            IsConstIterator>::move_to_left_most_child (index_pointer node) {

            std::shared_ptr<void const> store_node;
            while (!node.is_leaf ()) {
                visited_parents_.push ({node, 0});
                if (visited_parents_.size () <= details::max_internal_depth) {
                    internal_node const * internal = nullptr;
                    std::tie (store_node, internal) = internal_node::get_node (index_->db (), node);
                    assert (!store_node || store_node.get () == internal);
                    node = (*internal)[0];
                } else {
                    linear_node const * linear = nullptr;
                    std::tie (store_node, linear) = linear_node::get_node (index_->db (), node);
                    node = (*linear)[0];
                }
            }

            // Push the leaf node in the top of stack.
            visited_parents_.push ({node});
        }


        //*  _              _                      *
        //* | |_  __ _ _ __| |_   _ __  __ _ _ __  *
        //* | ' \/ _` | '  \  _| | '  \/ _` | '_ \ *
        //* |_||_\__,_|_|_|_\__| |_|_|_\__,_| .__/ *
        //*                                 |_|    *
        // hamt_map::clear
        // ~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        void hamt_map<KeyType, ValueType, Hash, KeyEqual>::clear (index_pointer node,
                                                                  unsigned shifts) {
            assert (node.is_heap () && !node.is_leaf ());
            if (details::depth_is_internal_node (shifts)) {
                auto internal = node.untag_node<internal_node *> ();
                // Recursively release the children of this internal node.
                for (auto p : *internal) {
                    if (p.is_heap ()) {
                        this->clear (p, shifts + details::hash_index_bits);
                    }
                }
            }

            this->delete_node (node, shifts);
        }

        // hamt_map::load_leaf_node
        // ~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::load_leaf_node (address const addr) const
            -> value_type {

            serialize::archive::database_reader archive (db_, addr);
            return serialize::read<std::pair<KeyType, ValueType>> (archive);
        }

        // hamt_map::get_key
        // ~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::get_key (address const addr) const
            -> key_type {

            serialize::archive::database_reader archive (db_, addr);
            return serialize::read<KeyType> (archive);
        }

        // hamt_map::store_leaf_node
        // ~~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename OtherValueType>
        address hamt_map<KeyType, ValueType, Hash, KeyEqual>::store_leaf_node (
            transaction_base & transaction, OtherValueType const & v,
            not_null<parent_stack *> parents) {

            assert (&db_ == &transaction.db ());
            auto writer = serialize::archive::make_writer (transaction);
            // make sure the alignment of leaf node is 4.
            transaction.alloc_rw (0, 4);
            // Now write the node and return where it went.
            address result = serialize::write (writer, v);
            parents->push ({result});

            return result;
        }



        // hamt_map::insert_into_leaf
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename OtherValueType>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::insert_into_leaf (
            transaction_base & transaction, index_pointer const & existing_leaf,
            OtherValueType const & new_leaf, hash_type existing_hash, hash_type hash,
            unsigned shifts, not_null<parent_stack *> parents) -> index_pointer {

            if (details::depth_is_internal_node (shifts)) {
                auto new_hash = hash & details::hash_index_mask;
                auto old_hash = existing_hash & details::hash_index_mask;
                if (new_hash != old_hash) {
                    address leaf_addr = this->store_leaf_node (transaction, new_leaf, parents);
                    auto internal_ptr =
                        new internal_node (existing_leaf, leaf_addr, old_hash, new_hash);
                    auto new_leaf_index = internal_node::get_new_index (new_hash, old_hash);
                    parents->push ({internal_ptr, new_leaf_index});
                    return internal_ptr;
                }

                // We've found a (partial) hash collision. Replace this leaf node with an internal
                // node. The existing key must be replaced with a sub-hash table and the next 6 bit
                // hash of the existing key computed. If there's still a collision, we repeat the
                // process. The new and existing keys are then inserted in the new sub-hash table.
                // As long as the partial hashes match, we have to create single element internal
                // nodes to represent them. This should happen very rarely with a reasonably good
                // hash function.

                shifts += details::hash_index_bits;
                hash >>= details::hash_index_bits;
                existing_hash >>= details::hash_index_bits;

                index_pointer leaf_ptr = this->insert_into_leaf (
                    transaction, existing_leaf, new_leaf, existing_hash, hash, shifts, parents);
                auto internal_ptr =
                    std::unique_ptr<internal_node> (new internal_node (leaf_ptr, old_hash));
                parents->push ({internal_ptr.get (), 0});
                return index_pointer{internal_ptr.release ()};
            }

            // We ran out of hash bits: create a new linear node.
            std::unique_ptr<linear_node> linear_ptr = linear_node::allocate (
                existing_leaf.addr, this->store_leaf_node (transaction, new_leaf, parents));
            parents->push ({linear_ptr.get (), 1});
            return index_pointer{linear_ptr.release ()};
        }

        // hamt_map::delete_node
        // ~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        void hamt_map<KeyType, ValueType, Hash, KeyEqual>::delete_node (index_pointer node,
                                                                        unsigned shifts) {
            if (node.is_heap ()) {
                assert (!node.is_leaf ());
                if (details::depth_is_internal_node (shifts)) {
                    delete node.untag_node<internal_node *> ();
                } else {
                    delete node.untag_node<linear_node *> ();
                }
            }
        }

        // hamt_map::insert_into_internal
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename OtherValueType>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::insert_into_internal (
            transaction_base & transaction, index_pointer node, OtherValueType const & value,
            hash_type hash, unsigned shifts, not_null<parent_stack *> parents, bool is_upsert)
            -> std::pair<index_pointer, bool> {

            std::shared_ptr<internal_node const> iptr;
            internal_node const * internal = nullptr;
            std::tie (iptr, internal) = internal_node::get_node (transaction.db (), node);
            assert (internal != nullptr);

            // Now work out which of the children we're going to be visiting next.
            index_pointer child_slot;
            auto index = std::size_t{0};
            std::tie (child_slot, index) = internal->lookup (hash & details::hash_index_mask);

            // If this slot isn't used, then ensure the node is on the heap, write the new leaf node
            // and point to it.
            if (index == details::not_found) {
                std::unique_ptr<internal_node> new_node;
                internal_node * inode = nullptr;
                std::tie (new_node, inode) = internal_node::make_writable (node, *internal);

                inode->insert_child (hash, this->store_leaf_node (transaction, value, parents),
                                     parents);
                new_node.release ();
                return {inode, false};
            }

            shifts += details::hash_index_bits;
            hash >>= details::hash_index_bits;

            // update child_slot
            bool key_exists;
            index_pointer new_child;
            std::tie (new_child, key_exists) = this->insert_node (transaction, child_slot, value,
                                                                  hash, shifts, parents, is_upsert);

            // If the insertion resulted in our child node being reallocated, then this node needs
            // to be heap-allocated and the child reference updated. The original child pointer may
            // also need to be freed.

            std::unique_ptr<internal_node> new_node;
            if (new_child != child_slot) {
                internal_node * inode = nullptr;
                std::tie (new_node, inode) = internal_node::make_writable (node, *internal);

                // Release a previous heap-allocated instance.
                index_pointer & child = (*inode)[index];
                this->delete_node (child, shifts);
                child = new_child;
                node = inode;
            }

            parents->push ({node, index});
            new_node.release ();
            return {node, key_exists};
        }

        // hamt_map::insert_into_linear
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename OtherValueType>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::insert_into_linear (
            transaction_base & transaction, index_pointer const node, OtherValueType const & value,
            not_null<parent_stack *> parents, bool is_upsert) -> std::pair<index_pointer, bool> {

            index_pointer result;
            bool key_exists = false;

            std::shared_ptr<linear_node const> lptr;
            linear_node const * orig_node = nullptr;
            std::tie (lptr, orig_node) = linear_node::get_node (transaction.db (), node);
            assert (orig_node != nullptr);

            index_pointer child_slot;
            auto index = std::size_t{0};

            std::tie (child_slot, index) =
                orig_node->lookup<KeyType> (transaction.db (), value.first, equal_);
            if (index == details::not_found) {
                // The key wasn't present in the node so we simply append it.
                // TODO: keep these entries sorted?

                // Load into memory with space for 1 new child node.
                std::unique_ptr<linear_node> new_node = linear_node::allocate_from (*orig_node, 1U);
                index = orig_node->size ();
                (*new_node)[index] = this->store_leaf_node (transaction, value, parents);
                result = new_node.release ();
            } else {
                key_exists = true;
                if (is_upsert) {
                    std::unique_ptr<linear_node> new_node;
                    linear_node * lnode = nullptr;

                    if (node.is_heap ()) {
                        // If the node is already on the heap then there's no need to reallocate it.
                        lnode = node.untag_node<linear_node *> ();
                        result = node;
                    } else {
                        // Load into memory but no extra space.
                        new_node = linear_node::allocate_from (*orig_node, 0U);
                        lnode = new_node.get ();
                        result = lnode;
                    }
                    (*lnode)[index] = this->store_leaf_node (transaction, value, parents);
                    new_node.release ();
                } else {
                    parents->push (details::parent_type{(*orig_node)[index]});

                    // We didn't modify the node so our return value is the original node index
                    // pointer.
                    result = node;
                }
            }

            parents->push (details::parent_type{result, index});
            return {result, key_exists};
        }

        // hamt_map::insert_node
        // ~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename OtherValueType>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::insert_node (
            transaction_base & transaction, index_pointer const node, OtherValueType const & value,
            hash_type hash, unsigned shifts, not_null<parent_stack *> parents, bool is_upsert)
            -> std::pair<index_pointer, bool> {

            index_pointer result;
            bool key_exists = false;
            if (node.is_leaf ()) {                                 // This node is a leaf node.
                key_type const existing_key = get_key (node.addr); // Read key.
                if (equal_ (value.first, existing_key)) {
                    if (is_upsert) {
                        result = this->store_leaf_node (transaction, value, parents);
                    } else {
                        parents->push ({node});
                        result = node;
                    }
                    key_exists = true;
                } else {
                    auto const existing_hash =
                        static_cast<hash_type> ((hash_ (existing_key) >> shifts));
                    result = this->insert_into_leaf (transaction, node, value, existing_hash, hash,
                                                     shifts, parents);
                }
            } else {
                // This node is an internal or a linear node.
                if (details::depth_is_internal_node (shifts)) {
                    std::tie (result, key_exists) = this->insert_into_internal (
                        transaction, node, value, hash, shifts, parents, is_upsert);
                } else {
                    std::tie (result, key_exists) =
                        this->insert_into_linear (transaction, node, value, parents, is_upsert);
                }
            }

            return std::make_pair (result, key_exists);
        }

        // hamt_map::insert_or_upsert
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename OtherValueType>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::insert_or_upsert (
            transaction_base & transaction, OtherValueType const & value, bool is_upsert)
            -> std::pair<iterator, bool> {

            parent_stack parents;
            if (this->empty ()) {
                root_ = this->store_leaf_node (transaction, value, &parents);
                return std::make_pair (iterator (std::move (parents), this), true);
            } else {
                parent_stack reverse_parents;
                bool key_exists = false;
                auto hash = static_cast<hash_type> (hash_ (value.first));
                std::tie (root_, key_exists) = this->insert_node (
                    transaction, root_, value, hash, 0 /* shifts */, &reverse_parents, is_upsert);
                while (!reverse_parents.empty ()) {
                    parents.push (reverse_parents.top ());
                    reverse_parents.pop ();
                }
                return std::make_pair (iterator (std::move (parents), this), !key_exists);
            }
        }

        // hamt_map::insert
        // ~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename OtherKeyType, typename OtherValueType, typename>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::insert (
            transaction_base & transaction, std::pair<OtherKeyType, OtherValueType> const & value)
            -> std::pair<iterator, bool> {
            return this->insert_or_upsert (transaction, value, false /*is_upsert*/);
        }

        // hamp_map::insert_or_assign
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename OtherKeyType, typename OtherValueType, typename>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::insert_or_assign (
            transaction_base & transaction, std::pair<OtherKeyType, OtherValueType> const & value)
            -> std::pair<iterator, bool> {
            return this->insert_or_upsert (transaction, value, true /*is_upsert*/);
        }

        // hamt_map::insert_or_assign
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename OtherKeyType, typename OtherValueType, typename>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::insert_or_assign (
            transaction_base & transaction, OtherKeyType const & key, OtherValueType const & value)
            -> std::pair<iterator, bool> {
            return this->insert_or_assign (transaction, std::make_pair (key, value));
        }

        // hamt_map::flush
        // ~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        address hamt_map<KeyType, ValueType, Hash, KeyEqual>::flush (transaction_base & transaction) {
            // If this is a leaf node, there's nothing to do. Just return its store address.
            if (root_.is_address ()) {
                return root_.addr;
            }

            assert (root_.is_internal ());
            auto internal = root_.untag_node<internal_node *> ();
            root_ = internal->flush (transaction, 0 /* shifts */);
            delete internal;
            return root_.addr;
        }

        // hamt_map::find
        // ~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename OtherKeyType, typename>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::find (OtherKeyType const & key) const
            -> const_iterator {
            if (empty ()) {
                return this->cend ();
            }

            auto hash = static_cast<hash_type> (hash_ (key));
            unsigned bit_shifts = 0;
            index_pointer node = root_;
            parent_stack parents;

            std::shared_ptr<void const> store_node;
            while (!node.is_leaf ()) {
                index_pointer child_node;
                auto index = std::size_t{0};

                if (details::depth_is_internal_node (bit_shifts)) {
                    // It's an internal node.
                    internal_node const * internal = nullptr;
                    std::tie (store_node, internal) = internal_node::get_node (db_, node);
                    std::tie (child_node, index) =
                        internal->lookup (hash & details::hash_index_mask);
                } else {
                    // It's a linear node.
                    linear_node const * linear = nullptr;
                    std::tie (store_node, linear) = linear_node::get_node (db_, node);
                    std::tie (child_node, index) = linear->lookup<KeyType> (db_, key, equal_);
                }

                if (index == details::not_found) {
                    return this->cend ();
                }
                parents.push ({node, index});

                // Go to next sub-trie level
                node = child_node;
                bit_shifts += details::hash_index_bits;
                hash >>= details::hash_index_bits;
            }
            // It's a leaf node.
            assert (node.is_leaf ());
            key_type const existing_key = get_key (node.addr);
            if (equal_ (existing_key, key)) {
                parents.push ({node});
                return const_iterator (std::move (parents), this);
            }
            return this->cend ();
        }

        // hamt_map::make_begin_iterator
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename IteratorType>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::make_begin_iterator () const
            -> IteratorType {

            auto result = IteratorType (parent_stack (), this);
            if (root_.internal) {
                result.move_to_left_most_child (root_);
            }
            return result;
        }

    } // namespace index
} // namespace pstore
#endif // PSTORE_HAMT_MAP_HPP
// eof: include/pstore/hamt_map.hpp
