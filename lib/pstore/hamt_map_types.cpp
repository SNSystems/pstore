//*  _                     _                             _                          *
//* | |__   __ _ _ __ ___ | |_   _ __ ___   __ _ _ __   | |_ _   _ _ __   ___  ___  *
//* | '_ \ / _` | '_ ` _ \| __| | '_ ` _ \ / _` | '_ \  | __| | | | '_ \ / _ \/ __| *
//* | | | | (_| | | | | | | |_  | | | | | | (_| | |_) | | |_| |_| | |_) |  __/\__ \ *
//* |_| |_|\__,_|_| |_| |_|\__| |_| |_| |_|\__,_| .__/   \__|\__, | .__/ \___||___/ *
//*                                             |_|          |___/|_|               *
//===- lib/pstore/hamt_map_types.cpp --------------------------------------===//
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
/// \file hamt_map_types.cpp
#include "pstore/hamt_map_types.hpp"
#include "pstore/transaction.hpp"

namespace pstore {
    namespace index {
        namespace details {

            //*  _ _                                  _      *
            //* | (_)_ _  ___ __ _ _ _   _ _  ___  __| |___  *
            //* | | | ' \/ -_) _` | '_| | ' \/ _ \/ _` / -_) *
            //* |_|_|_||_\___\__,_|_|   |_||_\___/\__,_\___| *
            //*                                              *

            linear_node::signature_type const linear_node::signature = {
                {'I', 'n', 'd', 'x', 'L', 'n', 'e', 'r'}};

            // operator new
            // ~~~~~~~~~~~~
            void * linear_node::operator new (std::size_t s, nchildren size) {
                (void) s;
                std::size_t const actual_bytes = linear_node::size_bytes (size.n);
                assert (actual_bytes >= s);
                return ::operator new (actual_bytes);
            }

            // operator delete
            // ~~~~~~~~~~~~~~~
            void linear_node::operator delete (void * p, nchildren /*size*/) {
                ::operator delete (p);
            }
            void linear_node::operator delete (void * p) {
                ::operator delete (p);
            }

            // (ctor)
            // ~~~~~~
            linear_node::linear_node (std::size_t size)
                    : signature_{signature}
                    , size_{size} {

                static_assert (std::is_standard_layout<linear_node>::value,
                               "linear_node must be standard-layout");
                static_assert (
                    alignof (linear_node) >= 4,
                    "linear_node must have alignment >= 4 to ensure the bottom two bits are 0");

                static_assert (offsetof (linear_node, signature_) == 0,
                               "offsetof (linear_node, signature_) must be 0");
                static_assert (offsetof (linear_node, size_) == 8,
                               "offsetof (linear_node, size_) must be 8");
                static_assert (offsetof (linear_node, leaves_) == 16,
                               "offset of the first child must be 16");

                std::fill_n (std::begin (leaves_), size, address::null ());
            }


            // allocate
            // ~~~~~~~~
            std::unique_ptr<linear_node> linear_node::allocate (std::size_t num_children,
                                                                linear_node const & from_node) {
                // Allocate the new node and fill in the basic fields.
                auto new_node = std::unique_ptr<linear_node> (new (nchildren{num_children})
                                                                  linear_node (num_children));

                std::size_t const num_to_copy = std::min (num_children, from_node.size ());
                auto src_begin = from_node.leaves_;
                auto src_end = src_begin + num_to_copy;
                // Note that the last argument is '&leaves[0]' rather than just 'leaves' to defeat
                // an MSVC debug assertion that thinks it knows how big the leaves_ array is...
                std::copy (src_begin, src_end, &new_node->leaves_[0]);

                return new_node;
            }

            std::unique_ptr<linear_node> linear_node::allocate (address a, address b) {
                auto result = std::unique_ptr<linear_node> (new (nchildren{2U}) linear_node (2U));
                (*result)[0] = a;
                (*result)[1] = b;
                return result;
            }

            // allocate_from
            // ~~~~~~~~~~~~~
            std::unique_ptr<linear_node> linear_node::allocate_from (linear_node const & orig_node,
                                                                     std::size_t extra_children) {
                return linear_node::allocate (orig_node.size () + extra_children, orig_node);
            }

            std::unique_ptr<linear_node> linear_node::allocate_from (database const & db,
                                                                     index_pointer const node,
                                                                     std::size_t extra_children) {
                std::pair<std::shared_ptr<linear_node const>, linear_node const *> p =
                    linear_node::get_node (db, node);
                assert (p.second != nullptr);
                return linear_node::allocate_from (*p.second, extra_children);
            }

            // get_node
            // ~~~~~~~~
            auto linear_node::get_node (database const & db, index_pointer const node)
                -> std::pair<std::shared_ptr<linear_node const>, linear_node const *> {

                if (node.is_heap ()) {
                    auto ptr = node.untag_node<linear_node const *> ();
                    assert (ptr->signature_ == signature);
                    return {nullptr, ptr};
                }

                // Read an existing node. First work out its size.
                address const addr = node.untag_internal_address ();
                std::shared_ptr<linear_node const> ln = db.template getro<linear_node> (addr);
                std::size_t const orig_size = ln->size ();

                std::size_t const in_store_size = linear_node::size_bytes (orig_size);

                // Now access the data block for this linear node.
                ln = std::static_pointer_cast<linear_node const> (db.getro (addr, in_store_size));
                if (ln->signature_ != signature) {
                    raise (pstore::error_code::index_corrupt);
                }
                return {ln, ln.get ()};
            }

            // flush
            // ~~~~~
            address linear_node::flush (transaction_base & transaction) const {
                std::size_t const num_bytes = this->size_bytes ();

                std::shared_ptr<void> ptr;
                address result;
                std::tie (ptr, result) = transaction.alloc_rw (num_bytes, alignof (linear_node));
                std::memcpy (ptr.get (), this, num_bytes);
                return result;
            }

            //*  _     _                     _                _      *
            //* (_)_ _| |_ ___ _ _ _ _  __ _| |  _ _  ___  __| |___  *
            //* | | ' \  _/ -_) '_| ' \/ _` | | | ' \/ _ \/ _` / -_) *
            //* |_|_||_\__\___|_| |_||_\__,_|_| |_||_\___/\__,_\___| *
            //*                                                      *

            internal_node::signature_type const internal_node::signature = {
                {'I', 'n', 't', 'e', 'r', 'n', 'a', 'l'}};

            internal_node::internal_node ()
                    : signature_{signature}
                    , bitmap_{0} {

                static_assert (std::is_standard_layout<internal_node>::value,
                               "internal internal_node must be standard-layout");
                static_assert (alignof (linear_node) >= 4, "internal_node must have alignment "
                                                           ">= 4 to ensure the bottom two bits "
                                                           "are 0");

                static_assert (offsetof (internal_node, signature_) == 0,
                               "offsetof (internal_node, signature) must be 0");
                static_assert (offsetof (internal_node, bitmap_) == 8,
                               "offsetof (internal_node, bitmap_) must be 8");
                static_assert (offsetof (internal_node, children_) == 16,
                               "offset of the first child must be 16.");
            }

            // ctor (one child)
            // ~~~~~~~~~~~~~~~~
            internal_node::internal_node (index_pointer const & leaf, hash_type hash)
                    : signature_{signature}
                    , bitmap_{hash_type{1} << hash}
                    , children_{{leaf}} {}

            // ctor (two children)
            // ~~~~~~~~~~~~~~~~~~~
            internal_node::internal_node (index_pointer const & existing_leaf,
                                          index_pointer const & new_leaf, hash_type existing_hash,
                                          hash_type new_hash)
                    : signature_{signature}
                    , bitmap_ (hash_type{1} << existing_hash | hash_type{1} << new_hash) {

                auto const index_a = get_new_index (new_hash, existing_hash);
                auto const index_b = static_cast<unsigned> (index_a == 0);

                assert ((index_a & 1) == index_a);
                assert ((index_b & 1) == index_b);
                assert (index_a != index_b);

                children_[index_a] = index_pointer{new_leaf};
                children_[index_b] = existing_leaf;
            }

            // copy ctor
            // ~~~~~~~~~
            internal_node::internal_node (internal_node const & rhs)
                    : signature_{signature}
                    , bitmap_{rhs.bitmap_} {

                auto first = std::begin (rhs.children_);
                auto out = std::copy (first, first + rhs.size (), std::begin (children_));
                std::fill (out, std::end (children_), address::null ());
            }

            // make_writable
            // ~~~~~~~~~~~~~
            std::pair<std::unique_ptr<internal_node>, internal_node *>
            internal_node::make_writable (index_pointer node, internal_node const & internal) {
                std::unique_ptr<internal_node> new_node;
                internal_node * inode = nullptr;
                if (node.is_heap ()) {
                    inode = node.untag_node<internal_node *> ();
                    assert (inode->signature_ == signature);
                } else {
                    new_node = std::make_unique<internal_node> (internal);
                    inode = new_node.get ();
                }
                return {std::move (new_node), inode};
            }

            // lookup
            // ~~~~~~
            auto internal_node::lookup (hash_type hash_index) const
                -> std::pair<index_pointer, std::size_t> {

                assert (hash_index < (hash_type{1} << hash_index_bits));
                index_pointer child;
                std::size_t index = not_found;

                auto const bit_pos = hash_type{1} << hash_index;
                if ((bitmap_ & bit_pos) != 0) {
                    index = bit_count::pop_count (bitmap_ & (bit_pos - 1U));
                    child = children_[index];
                }
                return {child, index};
            }

            // validate_after_load
            // ~~~~~~~~~~~~~~~~~~~
            /// Perform crude validation of the internal node that we've just read from the
            /// store.
            ///
            /// - We know that if this node is not on the heap then none of its immediate
            ///   children can be on the heap because creating a heap node causes all of its
            ///   parents to be recursively modified.
            /// - Child nodes (whether internal, linear, or leaf) will always be at a lower
            ///   store address than the parent node because they are written to the store
            ///   in depth-first order.
            /// - All child node addresses should be different.
            ///
            /// \param internal  The node to be validated.
            /// \param addr  The address from which this node was loaded.
            /// \return  A boolean indicating whether the validation was successful.

            bool internal_node::validate_after_load (internal_node const & internal,
                                                     address const addr) {
                if (internal.signature_ != signature) {
                    return false;
                }

                auto first = std::begin (internal);
                auto last = std::end (internal);
                while (first != last) {
                    index_pointer child = *(first++);
                    if (child.is_heap () || child.untag_internal_address () >= addr) {
                        return false;
                    }

                    for (auto it = first; it != last; ++it) {
                        if (child == *it) {
                            return false;
                        }
                    }
                }
                return true;
            }

            // read_node
            // ~~~~~~~~~
            auto internal_node::read_node (database const & db, address const addr)
                -> std::shared_ptr<internal_node const> {
                /// Sadly, loading an internal_node needs to done in three stages:
                /// 1. Load the basic structure
                /// 2. Calculate the actual size of the child pointer array
                /// 3. Load the complete structure along with its child pointer array
                auto base = std::static_pointer_cast<internal_node const> (
                    db.getro (addr, sizeof (internal_node) - sizeof (internal_node::children_)));

                if (base->get_bitmap () == 0) {
                    raise (error_code::index_corrupt, db.path ());
                }
                std::size_t const actual_size = internal_node::size_bytes (base->size ());
                assert (actual_size > sizeof (internal_node) - sizeof (internal_node::children_));
                auto resl =
                    std::static_pointer_cast<internal_node const> (db.getro (addr, actual_size));

                if (!validate_after_load (*resl, addr)) {
                    raise (error_code::index_corrupt, db.path ());
                }

                return resl;
            }

            // get_node
            // ~~~~~~~~
            // TODO: for consistency with linear nodes, shouldn't this be called load_node()?
            auto internal_node::get_node (database const & db, index_pointer const node)
                -> std::pair<std::shared_ptr<internal_node const>, internal_node const *> {

                if (node.is_heap ()) {
                    return {nullptr, node.untag_node<internal_node *> ()};
                }

                address const addr = node.untag_internal_address ();
                std::shared_ptr<internal_node const> store_internal = read_node (db, addr);
                return {store_internal, store_internal.get ()};
            }

            // insert_child
            // ~~~~~~~~~~~~
            void internal_node::insert_child (hash_type const hash, index_pointer const leaf,
                                              ::pstore::gsl::not_null<parent_stack *> parents) {
                auto const hash_index = hash & details::hash_index_mask;
                auto const bit_pos = hash_type{1} << hash_index;
                assert (bit_pos != 0); // guarantee that we didn't shift the bit to oblivion
                assert ((this->bitmap_ & bit_pos) == 0); // check that this slot is free

                // Compute the child index by counting the number of 1 bits below bit_pos.
                unsigned const index = bit_count::pop_count (this->bitmap_ & (bit_pos - 1));
                unsigned const old_size = bit_count::pop_count (this->bitmap_);
                assert (old_size < hash_size);
                assert (index <= old_size);

                // Move elements from [index..old_size) to [index+1..old_size+1)
                {
                    auto const children_span = ::pstore::gsl::make_span (children_);
                    auto const num_to_move = old_size - index;

                    auto const span = children_span.subspan (index, num_to_move);
                    auto const d_span = children_span.subspan (index + 1, num_to_move);
                    assert (d_span.last (0).data () <= children_span.last (0).data ());

                    std::move_backward (std::begin (span), std::end (span), std::end (d_span));
                }

                children_[index] = leaf;

                this->bitmap_ = this->bitmap_ | bit_pos;
                assert (bit_count::pop_count (this->bitmap_) == old_size + 1);
                parents->push ({this, index});
            }

            // store_node
            // ~~~~~~~~~~
            address internal_node::store_node (transaction_base & transaction) const {
                std::size_t const num_bytes = this->size_bytes (this->size ());

                std::shared_ptr<void> ptr;
                address result;
                std::tie (ptr, result) = transaction.alloc_rw (num_bytes, alignof (internal_node));
                std::memcpy (ptr.get (), this, num_bytes);
                return result;
            }

            // flush
            // ~~~~~
            address internal_node::flush (transaction_base & transaction, unsigned shifts) {
                auto const child_shifts = shifts + hash_index_bits;
                for (auto & p : *this) {
                    // If it is a heap node, flush its children first (depth-first search).
                    if (p.is_heap ()) {
                        if (child_shifts < max_hash_bits) { // internal node
                            auto internal = p.untag_node<internal_node *> ();
                            p.addr = internal->flush (transaction, child_shifts);
                            delete internal;
                        } else { // linear node
                            auto linear = p.untag_node<linear_node *> ();
                            p.addr = linear->flush (transaction) | internal_node_bit;
                            delete linear;
                        }
                    }
                }
                // Flush itself.
                auto addr = this->store_node (transaction);
                addr |= internal_node_bit;
                return addr;
            }

        } // namespace details
    }     // namespace index
} // namespace pstore
// eof: lib/pstore/hamt_map_types.cpp
