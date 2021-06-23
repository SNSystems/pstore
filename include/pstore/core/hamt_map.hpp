//===- include/pstore/core/hamt_map.hpp -------------------*- mode: C++ -*-===//
//*  _                     _                            *
//* | |__   __ _ _ __ ___ | |_   _ __ ___   __ _ _ __   *
//* | '_ \ / _` | '_ ` _ \| __| | '_ ` _ \ / _` | '_ \  *
//* | | | | (_| | | | | | | |_  | | | | | | (_| | |_) | *
//* |_| |_|\__,_|_| |_| |_|\__| |_| |_| |_|\__,_| .__/  *
//*                                             |_|     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file hamt_map.hpp

#ifndef PSTORE_CORE_HAMT_MAP_HPP
#define PSTORE_CORE_HAMT_MAP_HPP

#include "pstore/core/hamt_map_types.hpp"
#include "pstore/serialize/standard_types.hpp"

namespace pstore {

    class transaction_base;

    namespace index {

        inline index_base::~index_base () {}

#ifdef _WIN32
#    pragma warning(push)
#    pragma warning(disable : 4521)
#endif
        /// \brief A Hash array mapped trie index type for the pstore.
        ///
        /// \tparam KeyType  The map key type.
        /// \tparam ValueType  The map value type.
        /// \tparam Hash  A function which produces the hash of a supplied key. The signature must
        /// be compatible with:
        ///     index::details::hash_type(KeyType)
        /// \tparam KeyEqual  A function used to compare keys for equality. The signature must be
        /// compatible with:
        ///     bool(KeyType, KeyType)
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        class hamt_map final : public index_base {
            using hash_type = details::hash_type;
            using index_pointer = details::index_pointer;
            using internal_node = details::internal_node;
            using linear_node = details::linear_node;
            using parent_stack = details::parent_stack;

            /// A helper class which provides a member constant `value`` which is equal to true if
            /// types K and V have a serialized representation which is compatible with KeyType and
            /// ValueType respectively. Otherwise `value` is false.
            template <typename K, typename V>
            struct pair_types_compatible
                    : std::integral_constant<bool,
                                             serialize::is_compatible<KeyType, K>::value &&
                                                 serialize::is_compatible<ValueType, V>::value> {};

        public:
            using key_equal = KeyEqual;
            using key_type = KeyType;
            using mapped_type = ValueType;
            using value_type = std::pair<KeyType const, ValueType>;

            /// Inner class that describes a const_iterator and 'regular' iterator.
            template <bool IsConstIterator = true>
            class iterator_base {
                // Make iterator_base<true> a friend class of iterator_base<false> so the copy
                // constructor can access the private member variables.
                friend class iterator_base<true>;
                friend class hamt_map;
                using parent_stack =
                    typename hamt_map<KeyType, ValueType, Hash, KeyEqual>::parent_stack;
                using index_pointer =
                    typename hamt_map<KeyType, ValueType, Hash, KeyEqual>::index_pointer;

            public:
                using iterator_category = std::forward_iterator_tag;
                using value_type = hamt_map<KeyType, ValueType, Hash, KeyEqual>::value_type;
                using difference_type = std::ptrdiff_t;
                using pointer = value_type *;
                using reference = value_type &;

                /// For const_iterator: define value_reference_type to be a 'value_type const &'
                /// For iterator:       define value_reference_type to be a 'value_type &'
                using value_reference_type =
                    typename std::conditional<IsConstIterator, value_type const &,
                                              value_type &>::type;

                /// For const_iterator:   define value_pointer_type to be a 'value_type const *'
                /// For regular iterator: define value_pointer_type to be a 'value_type *'
                using value_pointer_type =
                    typename std::conditional<IsConstIterator, value_type const *,
                                              value_type *>::type;

                using database_reference =
                    typename std::conditional<IsConstIterator, database const &, database &>::type;

                iterator_base (database_reference db, parent_stack && parents, hamt_map const * idx)
                        : db_{db}
                        , visited_parents_ (std::move (parents))
                        , index_ (idx) {}

                iterator_base (iterator_base const & other) noexcept
                        : db_{other.db_}
                        , visited_parents_ (other.visited_parents_)
                        , index_ (other.index_) {
                    pos_.reset ();
                }

                /// Copy constructor. Allows for implicit conversion from a regular iterator to a
                /// const_iterator
                template <bool Enable = IsConstIterator,
                          typename = typename std::enable_if<Enable>::type>
                iterator_base (iterator_base<false> const & other) noexcept
                        : db_{other.db_}
                        , visited_parents_ (other.visited_parents_)
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

                bool operator!= (iterator_base const & other) const { return !operator== (other); }

                /// Dereference operator
                /// \return The value of the element to which this iterator is currently pointing.
                value_reference_type operator* () const {
                    if (pos_ == nullptr) {
                        pos_ = std::make_unique<value_type> (
                            index_->load_leaf_node (db_, this->get_address ()));
                    }
                    return *pos_;
                }

                value_pointer_type operator-> () const { return &operator* (); }

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
                    PSTORE_ASSERT (visited_parents_.size () >= 1);
                    details::parent_type const & parent = visited_parents_.top ();
                    PSTORE_ASSERT (parent.node.is_leaf () && parent.position == details::not_found);
                    return parent.node.addr;
                }

            private:
                void increment_internal_node ();

                /// Walks the iterator's position to point to the deepest, left-most leaf of the
                /// the current node. The iterator must be positing to an internal node when this
                /// method is called.
                void move_to_left_most_child (index_pointer node);

                unsigned get_shift_bits () const {
                    PSTORE_ASSERT (visited_parents_.size () >= 1);
                    return static_cast<unsigned> ((visited_parents_.size () - 1) *
                                                  details::hash_index_bits);
                }

                database_reference db_;
                parent_stack visited_parents_;
                hamt_map const * index_;
                mutable std::unique_ptr<value_type> pos_;
            };

            using iterator = iterator_base<false>;
            using const_iterator = iterator_base<true>;

            /// An associative container that contains key-value pairs with unique keys.
            ///
            /// \param db A database to which the index belongs.
            /// \param ip The index root address.
            /// \param hash A function that yields a hash from the key value.
            /// \param equal A function used to compare keys for equality.
            hamt_map (database const & db,
                      typed_address<header_block> ip = typed_address<header_block>::null (),
                      Hash const & hash = Hash (), KeyEqual const & equal = KeyEqual ());

            ~hamt_map () override { this->clear (); }

            /// \name Iterators
            ///@{

            range<database, hamt_map, iterator> make_range (database & db) { return {db, *this}; }
            range<database const, hamt_map const, const_iterator>
            make_range (database const & db) const {
                return {db, *this};
            }

            /// Returns an iterator to the beginning of the container
            iterator begin (database & db) { return make_begin_iterator (db, *this); }
            const_iterator begin (database const & db) const {
                return make_begin_iterator (db, *this);
            }
            const_iterator cbegin (database const & db) const {
                return make_begin_iterator (db, *this);
            }

            /// Returns an iterator to the end of the container
            iterator end (database & db) { return make_end_iterator (db, *this); }
            const_iterator end (database const & db) const { return make_end_iterator (db, *this); }
            const_iterator cend (database const & db) const {
                return make_end_iterator (db, *this);
            }

            ///@}

            /// \name Capacity
            ///@{

            /// Checks whether the container is empty
            bool empty () const noexcept {
                PSTORE_ASSERT (root_.is_empty () == (size_ == 0));
                return size_ == 0;
            }

            /// Returns the number of elements
            std::size_t size () const noexcept { return size_; }
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

            /// \name Lookup
            ///@{

            /// Finds an element with key equivalent to \p key.
            ///
            /// \tparam OtherKeyType  A type whose serialized representation is compatible with
            /// KeyType.
            /// \param db  The database to which the index belongs.
            /// \param key  The key value of the element to be found.
            /// \return Iterator to an element with key equivalent to key. If not such element is
            ///         found, past-the end iterator it returned.
            template <typename OtherKeyType,
                      typename = typename std::enable_if<
                          serialize::is_compatible<OtherKeyType, KeyType>::value>::type>
            const_iterator find (database const & db, OtherKeyType const & key) const;

            /// Checks if there is an element with key equivalent to \p key in the container.
            ///
            /// \tparam OtherKeyType  A type whose serialized representation is compatible with
            /// KeyType.
            /// \param db  The database to which the index belongs.
            /// \param key  The key value of the element to be check.
            /// \return True if the element is present in the container, false otherwise.
            template <typename OtherKeyType,
                      typename = typename std::enable_if<
                          serialize::is_compatible<OtherKeyType, KeyType>::value>::type>
            bool contains (database const & db, OtherKeyType const & key) const {
                return this->find (db, key) != this->end (db);
            }
            ///@}

            /// Flush any modified index nodes to the store.
            ///
            /// \param transaction  The transaction to which the map will be written.
            /// \param generation The generation number to which the map will be written.
            /// \returns The address of the index root node.
            typed_address<header_block> flush (transaction_base & transaction, unsigned generation);

            /// \name Accessors
            /// Provide access to index internals.
            ///@{

            /// Read a leaf node from a store.
            value_type load_leaf_node (database const & db, address const addr) const;

            /// Returns the index root pointer.
            index_pointer root () const noexcept { return root_; }
            ///@}

        private:
            static constexpr std::array<std::uint8_t, 8> index_signature{
                {'I', 'n', 'd', 'x', 'H', 'e', 'd', 'r'}};

            /// Stores a key/value data pair.
            template <typename OtherValueType>
            address store_leaf_node (transaction_base & transaction, OtherValueType const & v,
                                     gsl::not_null<parent_stack *> parents);

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
            key_type get_key (database const & db, address const addr) const;

            /// Called when the trie's top-level loop has descended as far as a leaf node. We need
            /// to convert that to an internal node.
            template <typename OtherValueType>
            auto insert_into_leaf (transaction_base & transaction,
                                   index_pointer const & existing_leaf,
                                   OtherValueType const & new_leaf, hash_type existing_hash,
                                   hash_type hash, unsigned shifts,
                                   gsl::not_null<parent_stack *> parents) -> index_pointer;

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
                                       unsigned shifts, gsl::not_null<parent_stack *> parents,
                                       bool is_upsert) -> std::pair<index_pointer, bool>;

            template <typename OtherValueType>
            auto insert_into_linear (transaction_base & transaction, index_pointer const node,
                                     OtherValueType const & value,
                                     gsl::not_null<parent_stack *> parents, bool is_upsert)
                -> std::pair<index_pointer, bool>;

            /// Insert a new key/value pair into a existing node, which could be a leaf node, an
            /// internal store node or an internal heap node.
            template <typename OtherValueType>
            auto insert_node (transaction_base & transaction, index_pointer const node,
                              OtherValueType const & value, hash_type hash, unsigned shifts,
                              gsl::not_null<parent_stack *> parents, bool is_upsert)
                -> std::pair<index_pointer, bool>;

            template <typename Database, typename HamtMap,
                      typename Iterator =
                          typename inherit_const<Database, iterator, const_iterator>::type>
            static Iterator make_begin_iterator (Database && db, HamtMap && m);
            template <typename Database, typename HamtMap,
                      typename Iterator =
                          typename inherit_const<Database, iterator, const_iterator>::type>
            static Iterator make_end_iterator (Database && db, HamtMap && m);

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

            /// \brief Write the index header.
            /// The index header simply holds a check signature, the tree root, and remembers the
            /// tree size for us on restore.
            ///
            /// \param transaction  The transaction to which the header block will be written.
            /// \result  The address at which the header block was written.
            typed_address<header_block> write_header_block (transaction_base & transaction);

            static constexpr auto internal_nodes_per_chunk =
                std::size_t{256} * 1024 / sizeof (internal_node);

            /// Internal nodes are allocated using a "chunked-sequence". This allocates memory in
            /// lumps sufficent for internal_nodes_per_chunk entries. This is then consumed as new
            /// in-heap internal nodes are created.
            ///
            /// The effect of this is to significantly reduce the number of memory allocations
            /// performed by the index code under heavy insertion pressure and thus give us a little
            /// extra performance. The cost is that we may waste as much as one chunk's worth of
            /// memory (minus sizeof (internal_node)).
            ///
            // TODO: we allocate nodes at their maximum size even though they may only contain two
            // members. This is rather wasteful and will prevent us from moving to larger hash
            // sizes due to the bloated memory consumption.
            using internal_nodes_container =
                chunked_sequence<internal_node, internal_nodes_per_chunk,
                                 internal_node::size_bytes (details::hash_size)>;
            std::unique_ptr<internal_nodes_container> internals_container_;

            unsigned revision_;
            index_pointer root_;
            std::size_t size_ = 0;
            /// The function called to produce a hash for a given key.
            Hash hash_;
            /// The function used to compare keys for equality.
            key_equal equal_;
        };
#ifdef _WIN32
#    pragma warning(pop)
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
        auto
        hamt_map<KeyType, ValueType, Hash, KeyEqual>::iterator_base<IsConstIterator>::operator++ ()
            -> iterator_base & {
            pos_.reset ();
            PSTORE_ASSERT (!visited_parents_.empty ());
            this->increment_internal_node ();
            return *this;
        }


        // increment internal node
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
                    std::tie (node, internal) = internal_node::get_node (db_, parent.node);
                    PSTORE_ASSERT (internal != nullptr);
                    size = internal->size ();
                } else {
                    std::tie (node, linear) = linear_node::get_node (db_, parent.node);
                    PSTORE_ASSERT (linear != nullptr);
                    size = linear->size ();
                }

                PSTORE_ASSERT (!parent.node.is_leaf () && parent.position < size);
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
                        visited_parents_.push ({index_pointer{(*linear)[parent.position]}});
                    }
                }
            }
        }

        // move to left most child
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
                    std::tie (store_node, internal) = internal_node::get_node (db_, node);
                    PSTORE_ASSERT (!store_node || store_node.get () == internal);
                    node = (*internal)[0];
                } else {
                    linear_node const * linear = nullptr;
                    std::tie (store_node, linear) = linear_node::get_node (db_, node);
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
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        constexpr std::array<std::uint8_t, 8>
            hamt_map<KeyType, ValueType, Hash, KeyEqual>::index_signature;

        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        hamt_map<KeyType, ValueType, Hash, KeyEqual>::hamt_map (
            database const & db, typed_address<header_block> const pos, Hash const & hash,
            KeyEqual const & equal)
                : internals_container_{std::make_unique<internal_nodes_container> ()}
                , revision_{db.get_current_revision ()}
                , hash_{hash}
                , equal_{equal} {

            if (pos != typed_address<header_block>::null ()) {
                // 'pos' points to the index header block which gives us the tree root and size.
                std::shared_ptr<header_block const> const hb = db.getro (pos);
                // Check that this block appears to be sensible.
#if PSTORE_SIGNATURE_CHECKS_ENABLED
                if (hb->signature != index_signature) {
                    raise (pstore::error_code::index_corrupt);
                }
#endif

                {
                    auto const root = index_pointer{hb->root};
                    if (root.is_heap () || (hb->size == 0U && !root.is_empty ()) ||
                        (hb->size > 0U && root.is_empty ()) ||
                        (hb->size == 1U && !root.is_leaf ()) ||
                        (hb->size > 1U && !root.is_internal ())) {

                        raise (pstore::error_code::index_corrupt);
                    }
                }
                size_ = hb->size;
                root_ = hb->root;
            }
        }

        // hamt_map::clear
        // ~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        void hamt_map<KeyType, ValueType, Hash, KeyEqual>::clear (index_pointer node,
                                                                  unsigned shifts) {
            PSTORE_ASSERT (node.is_heap () && !node.is_leaf ());
            if (details::depth_is_internal_node (shifts)) {
                auto * const internal = node.untag_node<internal_node *> ();
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
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::load_leaf_node (database const & db,
                                                                           address const addr) const
            -> value_type {

            return serialize::read<std::pair<KeyType, ValueType>> (
                serialize::archive::database_reader{db, addr});
        }

        // hamt_map::get_key
        // ~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::get_key (database const & db,
                                                                    address const addr) const
            -> key_type {

            return serialize::read<KeyType> (serialize::archive::database_reader{db, addr});
        }

        // hamt_map::store_leaf_node
        // ~~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename OtherValueType>
        address hamt_map<KeyType, ValueType, Hash, KeyEqual>::store_leaf_node (
            transaction_base & transaction, OtherValueType const & v,
            gsl::not_null<parent_stack *> const parents) {

            // Make sure the alignment of leaf node is 4 to ensure that the two LSB are guaranteed
            // 0. If 'v' has greater alignment, serialize::write() will add additional padding.
            constexpr auto aligned_to = std::size_t{4};
            static_assert ((details::internal_node_bit | details::heap_node_bit) == aligned_to - 1,
                           "expected required alignment to be 4");
            transaction.allocate (0, aligned_to);

            // Now write the node and return where it went.
            address const result =
                serialize::write (serialize::archive::make_writer (transaction), v);
            PSTORE_ASSERT ((result.absolute () & (aligned_to - 1U)) == 0U);
            parents->push ({index_pointer{result}});

            return result;
        }

        // hamt_map::insert_into_leaf
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename OtherValueType>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::insert_into_leaf (
            transaction_base & transaction, index_pointer const & existing_leaf,
            OtherValueType const & new_leaf, hash_type existing_hash, hash_type hash,
            unsigned shifts, gsl::not_null<parent_stack *> parents) -> index_pointer {

            if (details::depth_is_internal_node (shifts)) {
                auto const new_hash = hash & details::hash_index_mask;
                auto const old_hash = existing_hash & details::hash_index_mask;
                if (new_hash != old_hash) {
                    address const leaf_addr =
                        this->store_leaf_node (transaction, new_leaf, parents);
                    auto const internal_ptr = index_pointer{
                        internal_node::allocate (internals_container_.get (), existing_leaf,
                                                 index_pointer{leaf_addr}, old_hash, new_hash)};
                    parents->push (
                        {internal_ptr, internal_node::get_new_index (new_hash, old_hash)});
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

                index_pointer const leaf_ptr = this->insert_into_leaf (
                    transaction, existing_leaf, new_leaf, existing_hash, hash, shifts, parents);
                auto const internal_ptr = index_pointer{
                    internal_node::allocate (internals_container_.get (), leaf_ptr, old_hash)};
                parents->push ({internal_ptr, 0U});
                return internal_ptr;
            }

            // We ran out of hash bits: create a new linear node.
            auto const linear_ptr = index_pointer{
                linear_node::allocate (existing_leaf.addr,
                                       this->store_leaf_node (transaction, new_leaf, parents))
                    .release ()};
            parents->push ({linear_ptr, 1U});
            return linear_ptr;
        }

        // hamt_map::delete_node
        // ~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        void hamt_map<KeyType, ValueType, Hash, KeyEqual>::delete_node (index_pointer const node,
                                                                        unsigned const shifts) {
            if (node.is_heap ()) {
                PSTORE_ASSERT (!node.is_leaf ());
                if (details::depth_is_internal_node (shifts)) {
                    // Internal nodes are owned by internals_container_. Don't delete them here. If
                    // this ever changes, then add something like: delete
                    // node.untag_node<internal_node *> ();
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
            hash_type hash, unsigned shifts, gsl::not_null<parent_stack *> parents, bool is_upsert)
            -> std::pair<index_pointer, bool> {

            std::shared_ptr<internal_node const> iptr;
            internal_node const * internal = nullptr;
            std::tie (iptr, internal) = internal_node::get_node (transaction.db (), node);
            PSTORE_ASSERT (internal != nullptr);

            // Now work out which of the children we're going to be visiting next.
            index_pointer child_slot;
            auto index = std::size_t{0};
            std::tie (child_slot, index) = internal->lookup (hash & details::hash_index_mask);

            // If this slot isn't used, then ensure the node is on the heap, write the new leaf node
            // and point to it.
            if (index == details::not_found) {
                internal_node * const inode =
                    internal_node::make_writable (internals_container_.get (), node, *internal);
                inode->insert_child (
                    hash, index_pointer{this->store_leaf_node (transaction, value, parents)},
                    parents);
                return {index_pointer{inode}, false};
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
                internal_node * const inode =
                    internal_node::make_writable (internals_container_.get (), node, *internal);

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
            gsl::not_null<parent_stack *> parents, bool const is_upsert)
            -> std::pair<index_pointer, bool> {

            index_pointer result;
            bool key_exists = false;

            std::shared_ptr<linear_node const> lptr;
            linear_node const * orig_node = nullptr;
            std::tie (lptr, orig_node) = linear_node::get_node (transaction.db (), node);
            PSTORE_ASSERT (orig_node != nullptr);

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
                    parents->push (details::parent_type{index_pointer{(*orig_node)[index]}});

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
            hash_type hash, unsigned shifts, gsl::not_null<parent_stack *> parents, bool is_upsert)
            -> std::pair<index_pointer, bool> {

            index_pointer result;
            bool key_exists = false;
            if (node.is_leaf ()) { // This node is a leaf node.
                key_type const existing_key = get_key (transaction.db (), node.addr); // Read key.
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

            database & db = transaction.db ();
            if (revision_ != db.get_current_revision ()) {
                raise (error_code::index_not_latest_revision);
            }

            parent_stack parents;
            if (this->empty ()) {
                root_ = this->store_leaf_node (transaction, value, &parents);
                size_ = 1;
                return std::make_pair (iterator (db, std::move (parents), this), true);
            }

            parent_stack reverse_parents;
            bool key_exists = false;
            auto hash = static_cast<hash_type> (hash_ (value.first));
            std::tie (root_, key_exists) = this->insert_node (
                transaction, root_, value, hash, 0 /* shifts */, &reverse_parents, is_upsert);
            while (!reverse_parents.empty ()) {
                parents.push (reverse_parents.top ());
                reverse_parents.pop ();
            }
            if (!key_exists) {
                ++size_;
            }
            return std::make_pair (iterator (db, std::move (parents), this), !key_exists);
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
        typed_address<header_block>
        hamt_map<KeyType, ValueType, Hash, KeyEqual>::flush (transaction_base & transaction,
                                                             unsigned const generation) {
            if (revision_ != transaction.db ().get_current_revision ()) {
                raise (error_code::index_not_latest_revision);
            }

            // If the root is a leaf node, there's nothing to do. If not, we start to recursively
            // flush the tree.
            if (!root_.is_address ()) {
                PSTORE_ASSERT (root_.is_internal ());
                root_ = root_.untag_node<internal_node *> ()->flush (transaction, 0 /*shifts*/);
                PSTORE_ASSERT (root_.is_address ());
                // Don't delete the internal node here. They are owned by internals_container_. If
                // this ever changes, then use something like 'delete internal' here.
            }

            auto const header_addr = this->size () > 0U ? this->write_header_block (transaction)
                                                        : typed_address<header_block>::null ();

            // Release all of the in-heap internal nodes that we have now flushed.
            internals_container_->clear ();

            // Update the revision number into which the index will be flushed.
            revision_ = generation;

            return header_addr;
        }

        // write header block
        // ~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        typed_address<header_block>
        hamt_map<KeyType, ValueType, Hash, KeyEqual>::write_header_block (
            transaction_base & transaction) {
            PSTORE_ASSERT (this->root ().is_address ());
            auto const pos = transaction.alloc_rw<header_block> ();
            pos.first->signature = index_signature;
            pos.first->size = this->size ();
            pos.first->root = this->root ().addr;
            return pos.second;
        }

        // hamt_map::find
        // ~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename OtherKeyType, typename>
        auto hamt_map<KeyType, ValueType, Hash, KeyEqual>::find (database const & db,
                                                                 OtherKeyType const & key) const
            -> const_iterator {
            if (empty ()) {
                return this->cend (db);
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
                    std::tie (store_node, internal) = internal_node::get_node (db, node);
                    std::tie (child_node, index) =
                        internal->lookup (hash & details::hash_index_mask);
                } else {
                    // It's a linear node.
                    linear_node const * linear = nullptr;
                    std::tie (store_node, linear) = linear_node::get_node (db, node);
                    std::tie (child_node, index) = linear->lookup<KeyType> (db, key, equal_);
                }

                if (index == details::not_found) {
                    return this->cend (db);
                }
                parents.push ({node, index});

                // Go to next sub-trie level
                node = child_node;
                bit_shifts += details::hash_index_bits;
                hash >>= details::hash_index_bits;
            }
            // It's a leaf node.
            PSTORE_ASSERT (node.is_leaf ());
            key_type const existing_key = get_key (db, node.addr);
            if (equal_ (existing_key, key)) {
                parents.push ({node});
                return const_iterator (db, std::move (parents), this);
            }
            return this->cend (db);
        }

        // hamt_map::make_begin_iterator
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename Database, typename HamtMap, typename Iterator>
        Iterator hamt_map<KeyType, ValueType, Hash, KeyEqual>::make_begin_iterator (Database && db,
                                                                                    HamtMap && m) {
            Iterator result{db, parent_stack (), &m};
            if (m.root_.internal) {
                result.move_to_left_most_child (m.root_);
            }
            return result;
        }

        // hamt_map::make_end_iterator
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~
        template <typename KeyType, typename ValueType, typename Hash, typename KeyEqual>
        template <typename Database, typename HamtMap, typename Iterator>
        Iterator hamt_map<KeyType, ValueType, Hash, KeyEqual>::make_end_iterator (Database && db,
                                                                                  HamtMap && m) {
            return {db, parent_stack (), &m};
        }

    } // namespace index
} // namespace pstore
#endif // PSTORE_CORE_HAMT_MAP_HPP
