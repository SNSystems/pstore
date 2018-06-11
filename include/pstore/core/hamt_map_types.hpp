//*  _                     _                             _                          *
//* | |__   __ _ _ __ ___ | |_   _ __ ___   __ _ _ __   | |_ _   _ _ __   ___  ___  *
//* | '_ \ / _` | '_ ` _ \| __| | '_ ` _ \ / _` | '_ \  | __| | | | '_ \ / _ \/ __| *
//* | | | | (_| | | | | | | |_  | | | | | | (_| | |_) | | |_| |_| | |_) |  __/\__ \ *
//* |_| |_|\__,_|_| |_| |_|\__| |_| |_| |_|\__,_| .__/   \__|\__, | .__/ \___||___/ *
//*                                             |_|          |___/|_|               *
//===- include/pstore/core/hamt_map_types.hpp -----------------------------===//
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
/// \file hamt_map_types.hpp

#ifndef PSTORE_HAMT_MAP_TYPES_HPP
#define PSTORE_HAMT_MAP_TYPES_HPP

#include <array>
#include <vector>

#include "pstore/core/array_stack.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/db_archive.hpp"
#include "pstore/core/file_header.hpp"
#include "pstore/support/bit_count.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    class transaction_base;

    namespace index {
        namespace details {
            using hash_type = std::uint64_t;

#ifdef _MSC_VER
            // TODO: VC2015RC won't allow pop_count to be constexpr. Sigh. This forces a (very
            // crude) population count implementation
            namespace details_count {
                constexpr inline unsigned bit_set (std::size_t v, unsigned bit) {
                    return (v & (std::size_t{1} << bit)) >> bit;
                }

                constexpr inline unsigned crude_pop_count (std::size_t v) {
                    return bit_set (v, 0x00) + bit_set (v, 0x01) + bit_set (v, 0x02) +
                           bit_set (v, 0x03) + bit_set (v, 0x04) + bit_set (v, 0x05) +
                           bit_set (v, 0x06) + bit_set (v, 0x07) + bit_set (v, 0x08) +
                           bit_set (v, 0x09) + bit_set (v, 0x0A) + bit_set (v, 0x0B) +
                           bit_set (v, 0x0C) + bit_set (v, 0x0D) + bit_set (v, 0x0E) +
                           bit_set (v, 0x0F);
                }
            } // namespace details_count
            constexpr unsigned hash_index_bits =
                details_count::crude_pop_count (sizeof (hash_type) * 8 - 1);
#else
            // TODO: one day the MSVC-specific code will go away and this will be all we need...
            constexpr unsigned hash_index_bits = bit_count::pop_count (sizeof (hash_type) * 8 - 1);
#endif

            constexpr auto const hash_size = sizeof (hash_type) * 8;
            constexpr auto const max_hash_bits =
                (hash_size + 7) / hash_index_bits * hash_index_bits;
            constexpr unsigned hash_index_mask = (1U << hash_index_bits) - 1U;
            constexpr unsigned max_internal_depth = max_hash_bits / hash_index_bits;

            /// The max depth of the hash trees include several levels internal nodes (max_hash_bits
            /// / hash_index_bits), one level linear node and one level leaf node.
            constexpr unsigned max_tree_depth = max_internal_depth + 2U;

            /// Using LSB for marking internal nodes
            constexpr std::uintptr_t internal_node_bit = 1;

            /// Using second LSB for marking newly allocated internal nodes
            constexpr std::uintptr_t heap_node_bit = 2;

        } // namespace details
    }     // namespace index
} // namespace pstore


namespace pstore {
    namespace index {

        using ::pstore::gsl::not_null;

        //*  _                _           _    _         _    *
        //* | |_  ___ __ _ __| |___ _ _  | |__| |___  __| |__ *
        //* | ' \/ -_) _` / _` / -_) '_| | '_ \ / _ \/ _| / / *
        //* |_||_\___\__,_\__,_\___|_|   |_.__/_\___/\__|_\_\ *
        //*                                                   *
        /// The address of an instance of this type is passed to the hamt_map ctor to load an
        /// existing index, and it is returned by a call to hamt_map::flush().
        struct header_block {
            std::array<std::uint8_t, 8> signature;
            /// The number of keys stored in the tree.
            std::uint64_t size;
            /// The store address of the tree's root node.
            address root;
        };

        PSTORE_STATIC_ASSERT (sizeof (header_block) == 24);
        PSTORE_STATIC_ASSERT (offsetof (header_block, signature) == 0);
        PSTORE_STATIC_ASSERT (offsetof (header_block, size) == 8);
        PSTORE_STATIC_ASSERT (offsetof (header_block, root) == 16);


        namespace details {

            std::size_t const not_found = std::numeric_limits<std::size_t>::max ();

            class internal_node;
            class linear_node;

            inline bool depth_is_internal_node (unsigned shift) {
                return shift < details::max_hash_bits;
            }


            //*  _         _                     _     _            *
            //* (_)_ _  __| |_____ __  _ __  ___(_)_ _| |_ ___ _ _  *
            //* | | ' \/ _` / -_) \ / | '_ \/ _ \ | ' \  _/ -_) '_| *
            //* |_|_||_\__,_\___/_\_\ | .__/\___/_|_||_\__\___|_|   *
            //*                       |_|                           *
            /// An index pointer is either a database address or a pointer to volatile RAM.
            /// The type information (which of the two fields applies) is carried externally.
            union index_pointer {
                index_pointer ()
                        : internal{nullptr} {}
                index_pointer (address const a)
                        : addr (a) {}
                index_pointer (internal_node * p)
                        : internal{tag_node (p)} {}
                index_pointer (linear_node * p)
                        : linear{tag_node (p)} {}
                index_pointer (index_pointer const &) = default;
                index_pointer (index_pointer &&) noexcept = default;

                index_pointer & operator= (index_pointer const &) = default;
                index_pointer & operator= (index_pointer &&) noexcept = default;
                index_pointer & operator= (address const & a) {
                    addr = a;
                    return *this;
                }
                index_pointer & operator= (internal_node * p) {
                    internal = tag_node (p);
                    return *this;
                }
                index_pointer & operator= (linear_node * l) {
                    linear = tag_node (l);
                    return *this;
                }


                bool operator== (index_pointer const & other) const { return addr == other.addr; }
                bool operator!= (index_pointer const & other) const { return !operator== (other); }

                operator bool () const { return !this->is_empty (); }

                /// Returns true if the index_pointer is pointing to an internal node, false
                /// otherwise.
                /// \sa is_leaf
                bool is_internal () const {
                    return (reinterpret_cast<std::uintptr_t> (internal) & internal_node_bit) != 0;
                }

                /// Returns true if the index_pointer is pointing to a linear node, false otherwise.
                /// \sa is_leaf
                bool is_linear () const { return is_internal (); }

                /// Returns true if the index_pointer is pointing to a value address in the store,
                /// false otherwise.
                /// \sa is_internal
                bool is_leaf () const { return !is_internal (); }

                /// Returns true if the index_pointer is pointing to a heap node, false otherwise.
                /// \sa is_addr
                bool is_heap () const {
                    return (reinterpret_cast<std::uintptr_t> (internal) & heap_node_bit) != 0;
                }

                /// Returns true if the index_pointer is pointing to a store node, false otherwise.
                /// \sa is_heap
                bool is_address () const { return !is_heap (); }

                bool is_empty () const { return internal == nullptr; }
                template <typename T>
                T tag_node () const {
                    return reinterpret_cast<T> (tag ());
                }

                template <typename T>
                T untag_node () const {
                    return reinterpret_cast<T> (untag ());
                }

                address untag_internal_address () const {
                    return address::make (addr.absolute () & ~internal_node_bit);
                }

                address addr;
                internal_node * internal;
                linear_node * linear;

            private:
                static std::uintptr_t tag (void * p) {
                    return reinterpret_cast<std::uintptr_t> (p) | internal_node_bit | heap_node_bit;
                }
                static inline internal_node * tag_node (internal_node * p) {
                    return reinterpret_cast<internal_node *> (tag (p));
                }
                static inline linear_node * tag_node (linear_node * p) {
                    return reinterpret_cast<linear_node *> (tag (p));
                }

                std::uintptr_t tag () const { return tag (internal); }

                std::uintptr_t untag () const {
                    return reinterpret_cast<std::uintptr_t> (internal) & ~internal_node_bit &
                           ~heap_node_bit;
                }
            };


            PSTORE_STATIC_ASSERT (sizeof (index_pointer) == 8);
            PSTORE_STATIC_ASSERT (offsetof (index_pointer, addr) == 0);
            PSTORE_STATIC_ASSERT (sizeof (index_pointer::internal) == sizeof (index_pointer::addr));
            PSTORE_STATIC_ASSERT (offsetof (index_pointer, internal) == 0);
            PSTORE_STATIC_ASSERT (sizeof (index_pointer::linear) == sizeof (index_pointer::addr));
            PSTORE_STATIC_ASSERT (offsetof (index_pointer, linear) == 0);


            //*                         _     _                   *
            //*  _ __  __ _ _ _ ___ _ _| |_  | |_ _  _ _ __  ___  *
            //* | '_ \/ _` | '_/ -_) ' \  _| |  _| || | '_ \/ -_) *
            //* | .__/\__,_|_| \___|_||_\__|  \__|\_, | .__/\___| *
            //* |_|                               |__/|_|         *
            /// \brief A class used to keep the pointer to parent node and the child slot.
            struct parent_type {
                parent_type () = default;

                /// Constructs a parent type object.
                /// \param idx  The pointer to either the parent node or a leaf node.
                /// \param pos  If idx is a leaf node address, pos is set to the default value
                ///             (not_found). Otherwise, pos refers to the child slot.
                parent_type (index_pointer const idx, std::size_t pos = not_found)
                        : node (idx)
                        , position (pos) {}

                bool operator== (parent_type const & other) const {
                    return position == other.position && node == other.node;
                }
                bool operator!= (parent_type const & other) const { return !operator== (other); }

                index_pointer node;
                std::size_t position = 0;
            };

            using parent_stack = array_stack<parent_type, max_tree_depth>;


            //*  _ _                                  _      *
            //* | (_)_ _  ___ __ _ _ _   _ _  ___  __| |___  *
            //* | | | ' \/ -_) _` | '_| | ' \/ _ \/ _` / -_) *
            //* |_|_|_||_\___\__,_|_|   |_||_\___/\__,_\___| *
            //*                                              *
            /// \brief A linear node.
            /// Linear nodes as used as the place of last resort for entries which cannot be
            /// distinguished by their hash value.
            class linear_node {
            public:
                using iterator = address *;
                using const_iterator = address const *;

                void * operator new (std::size_t) = delete;
                void operator delete (void * p);

                ~linear_node () = default;
                linear_node & operator= (linear_node const & rhs) = delete;

                /// \name Construction
                ///@{

                /// \brief Allocates a new linear node in memory and copy the contents of an
                /// existing node into it. The new node is allocated with sufficient storage for the
                /// child of the supplied node plus the number passed in the 'extra_children'
                /// parameter.
                ///
                /// \param orig_node  A node whose contents will be copied into the newly allocated
                /// linear node.
                /// \param extra_children  The number of extra child for which space will be
                /// allocated. This number is added to the number of children in 'orig_node' in
                /// calculating the amount of storage to be allocated.
                /// \result  A pointer to the newly allocated linear node.
                static std::unique_ptr<linear_node> allocate_from (linear_node const & orig_node,
                                                                   std::size_t extra_children);

                /// \brief Allocates a new in-memory linear node based on the contents of an
                /// existing store node.
                ///
                /// \param db The database from which the source node should be loaded.
                /// \param node A reference to the source node which may be either in-heap or
                /// in-store.
                /// \param extra_children The number of additional child nodes for which storage
                /// should be allocated.
                /// \result  A pointer to the newly allocated linear node.
                static std::unique_ptr<linear_node> allocate_from (database const & db,
                                                                   index_pointer const node,
                                                                   std::size_t extra_children);

                /// \brief Allocates a new linear node in memory with sufficient space for two leaf
                /// addresses.
                ///
                /// \param a  The first leaf address for the new linear node.
                /// \param b  The second leaf address for the new linear node.
                /// \result  A pointer to the newly allocated linear node.
                static std::unique_ptr<linear_node> allocate (address a, address b);

                /// \brief Returns a pointer to a linear node which may be in-heap or in-store.
                ///
                /// If the supplied index_pointer points to a heap-resident linear node then returns
                /// a pair whose first member is nullptr and whose second member contains the node
                /// pointer. If the index_pointer references an in-store linear node then the node
                /// is fetched and the function returns a pair whose first member is the store's
                /// shared_ptr and whose second member is the equivalent raw pointer (i.e.
                /// result.first.get () == result.second). In this case, the second pointer is only
                /// valid as long as the first pointer is "live".
                ///
                /// \param db The database from which the node should be loaded.
                /// \param node A pointer to the node location: either in the heap or in the store.
                /// \result A pair holding a pointer to the node in-store memory (if necessary) and
                /// its raw pointer.
                static auto get_node (database const & db, index_pointer const node)
                    -> std::pair<std::shared_ptr<linear_node const>, linear_node const *>;
                ///@}

                /// \name Element access
                ///@{
                address operator[] (std::size_t i) const {
                    assert (i < size_);
                    return leaves_[i];
                }
                address & operator[] (std::size_t i) {
                    assert (i < size_);
                    return leaves_[i];
                }
                ///@}

                /// \name Iterators
                ///@{

                iterator begin () { return leaves_; }
                const_iterator begin () const { return leaves_; }
                const_iterator cbegin () const { return this->begin (); }

                iterator end () { return leaves_ + size_; }
                const_iterator end () const { return leaves_ + size_; }
                const_iterator cend () const { return this->end (); }
                ///@}


                /// \name Capacity
                ///@{

                /// Checks whether the container is empty.
                bool empty () const { return size_ == 0; }
                /// Returns the number of elements.
                std::size_t size () const { return size_; }
                ///@}

                /// \name Storage
                ///@{

                /// Returns the number of bytes of storage required for the node.
                std::size_t size_bytes () const { return linear_node::size_bytes (this->size ()); }

                /// Returns the number of bytes of storage required for a linear node with 'size'
                /// children.
                static std::size_t size_bytes (std::uint64_t size) {
                    return sizeof (linear_node) - sizeof (linear_node::leaves_) +
                           sizeof (linear_node::leaves_[0]) * size;
                }
                ///@}

                /// Write this linear node to the store.
                ///
                /// \param transaction The transaction to which the linear node will be appended.
                /// \result The address at which the node was written.
                address flush (transaction_base & transaction) const;

                /// Search the linear node and return the child slot if the key exists.
                /// Otherwise, return the {nullptr, not_found} pair.
                /// \tparam KeyType The type of the keys stored in the linear node.
                /// \tparam OtherKeyType  A type whose serialized value is compatible with KeyType
                /// \tparam KeyEqual  The type of the key-comparison function.
                /// \param db  The dataase instance from which child nodes should be loaded.
                /// \param key  The key to be located.
                /// \param equal  A comparison function which will be called to compare child nodes
                /// to the supplied key value. It should return true if the keys match and false
                /// otherwise.
                /// \result If found, returns an `index_pointer` reference to the child node and the
                /// position within the linear node instance of the child record. If not found,
                /// returns the pair index_pointer (), details::not_found.

                template <typename KeyType, typename OtherKeyType, typename KeyEqual,
                          typename = typename std::enable_if<
                              serialize::is_compatible<KeyType, OtherKeyType>::value>::type>
                auto lookup (database & db, OtherKeyType const & key, KeyEqual equal) const
                    -> std::pair<index_pointer const, std::size_t>;

            private:
                using signature_type = std::array<std::uint8_t, 8>;
                static signature_type const signature;

                struct nchildren {
                    std::size_t n;
                };
                /// A placement-new implementation which allocates sufficient storage for a linear
                /// node with the number of children given by the size parameter.
                void * operator new (std::size_t s, nchildren size);
                void operator delete (void * p, nchildren size);

                /// \param size The capacity of this linear node.
                explicit linear_node (std::size_t size);

                /// Allocates a new linear node in memory.
                ///
                /// \param num_children Sufficient space is allocated for the number of child nodes
                /// specified in this parameter.
                /// \param from_node A node whose contents will be copied into the new node. If the
                /// number of children requested is greater than the number of children in
                /// from_node, the remaining entries are zeroed; if less then the child node
                /// collection is truncated after the specified number of entries.
                /// \result A pointer to the newly allocated linear node.
                static std::unique_ptr<linear_node> allocate (std::size_t num_children,
                                                              linear_node const & from_node);

                signature_type signature_;
                std::uint64_t size_;
                address leaves_[1];
            };

            // lookup
            // ~~~~~~
            template <typename KeyType, typename OtherKeyType, typename KeyEqual, typename>
            auto linear_node::lookup (database & db, OtherKeyType const & key, KeyEqual equal) const
                -> std::pair<index_pointer const, std::size_t> {
                // Linear search. TODO: perhaps we should sort the nodes and use a binary
                // search? This would require a template compare method.
                std::size_t cnum = 0;
                for (auto const & child : *this) {
                    KeyType const existing_key =
                        serialize::read<KeyType> (serialize::archive::database_reader{db, child});
                    if (equal (existing_key, key)) {
                        return {index_pointer{child}, cnum};
                    }
                    ++cnum;
                }
                // Not found
                return {index_pointer (), details::not_found};
            }

            //*  _     _                     _                _      *
            //* (_)_ _| |_ ___ _ _ _ _  __ _| |  ___  ___  __| |___  *
            //* | | ' \  _/ -_) '_| ' \/ _` | | |   \/ _ \/ _` / -_) *
            //* |_|_||_\__\___|_| |_||_\__,_|_| |_|\_\___/\__,_\___| *
            //*                                                      *
            /// An internal trie node.
            class internal_node {
                using children_container = std::array<index_pointer, hash_size>;

            public:
                internal_node ();
                /// Construct an internal node with a child.
                internal_node (index_pointer const & leaf, hash_type hash);

                /// Construct the internal node with two children.
                internal_node (index_pointer const & existing_leaf, index_pointer const & new_leaf,
                               hash_type existing_hash, hash_type new_hash);

                internal_node (internal_node const & rhs);

                ~internal_node () = default;
                internal_node & operator= (internal_node const & rhs) = delete;

                /// Return the internal heap node pointer if node is a heap internal node. Otherwise
                /// return the pointer which is pointed to the store node.
                ///
                /// \return A pair of which the first element is a store-pointer to the node body
                /// with the store address space. This may be null if called on a heap-resident
                /// node. The second element is the raw node pointer, that is, the address of a heap
                /// node or the result of calling .get() on the store-pointer.
                static auto get_node (database const & db, index_pointer const node)
                    -> std::pair<std::shared_ptr<internal_node const>, internal_node const *>;
                /// Return the internal store node pointer.
                static auto read_node (database const & db, address const addr)
                    -> std::shared_ptr<internal_node const>;

                /// Returns a writable reference to an internal node. If the \p node parameter
                /// references an in-heap node, then this pointer is returned otherwise a copy of
                /// the \p internal parameter is placed in heap-allocated memory.
                ///
                /// \note It is expected that both \p node and \p internal are references to the
                /// same node.
                ///
                /// \param node A reference to an internal node. This may be either in-store on the
                /// heap. If on the heap the returned value is the underlying pointer.
                /// \param internal  A read-only instance of an internal node. If the \p node
                /// parameter is in-store then a copy of this value is placed on the heap.
                /// \result  See above.

                static std::pair<std::unique_ptr<internal_node>, internal_node *>
                make_writable (index_pointer node, internal_node const & internal);


                /// Computes the number of bytes occupied by the in-store representation of an
                /// internal node with the given number of children.
                ///
                /// \param num_children  The number of children to assume for the purpose of
                /// computing the number of bytes occupied.
                /// \return The number of bytes occupied by an in-store internal node with the given
                /// number of child nodes.
                static std::size_t size_bytes (unsigned num_children) {
                    assert (num_children > 0 && num_children < max_hash_bits);
                    return sizeof (internal_node) - sizeof (internal_node::children_) +
                           sizeof (decltype (internal_node::children_)::value_type) * num_children;
                }

                unsigned size () const {
                    assert (this->bitmap_ != hash_type{0});
                    unsigned const result = bit_count::pop_count (this->bitmap_);
                    assert (result <= children_.size ());
                    return result;
                }

                /// Return the new leaf child index number.
                static unsigned get_new_index (hash_type new_hash, hash_type existing_hash) {
                    return static_cast<unsigned> (new_hash >= existing_hash);
                }

                std::pair<index_pointer, std::size_t> lookup (hash_type hash_index) const;

                /// Insert a child into the internal node (this).
                void insert_child (hash_type const hash, index_pointer const leaf,
                                   not_null<parent_stack *> parents);

                /// Write an internal node and its children into a store.
                address flush (transaction_base & transaction, unsigned shifts);


                index_pointer const & operator[] (std::size_t i) const {
                    assert (i < size ());
                    return children_[i];
                }

                index_pointer & operator[] (std::size_t i) {
                    assert (i < size ());
                    return children_[i];
                }

                hash_type get_bitmap () const { return bitmap_; }
                /// A function for deliberately creating illegal internal nodes in the unit test. DO
                /// NOT USE except for that purpose!
                void set_bitmap (hash_type bm) { bitmap_ = bm; }

                using iterator = children_container::iterator;
                using const_iterator = children_container::const_iterator;

                /// \name Iterators
                ///@{

                iterator begin () { return std::begin (children_); }
                iterator end () { return this->begin () + this->size (); }
                const_iterator begin () const { return std::begin (children_); }
                const_iterator end () const { return this->begin () + this->size (); }
                const_iterator cbegin () const { return std::begin (children_); }
                const_iterator cend () const { return this->cbegin () + this->size (); }
                ///@}

            private:
                static bool validate_after_load (internal_node const & internal,
                                                 address const addr);

                /// Appends the internal node (which refers to a node in heap memory) to the
                /// store. Returns a new (in-store) internal store address.
                address store_node (transaction_base & transaction) const;

                using signature_type = std::array<std::uint8_t, 8>;
                static signature_type const signature;

                /// A magic number for internal nodes in the store. Acts as a quick integrity test
                /// for the index structures.
                signature_type signature_;

                /// For each index in the children array, the corresponding bit is set in this field
                /// if it is a reference to an internal node or an leaf node. In a linear node, the
                /// bitmap field contains the number of elements in the array.
                hash_type bitmap_;

                /// \brief The array of child node references.
                /// Each child may be in-memory or in-store. This array is declared as though it is
                /// fully populated with children, but the in-store representation only uses the
                /// number of elements that are actually in use.
                children_container children_;
            };

        } // namespace details
    }     // namespace index
} // namespace pstore

#endif // PSTORE_HAMT_MAP_TYPES_HPP
// eof: include/pstore/core/hamt_map_types.hpp
