//*      _ _  __  __  *
//*   __| (_)/ _|/ _| *
//*  / _` | | |_| |_  *
//* | (_| | |  _|  _| *
//*  \__,_|_|_| |_|   *
//*                   *
//===- include/pstore/core/diff.hpp ---------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_CORE_DIFF_HPP
#define PSTORE_CORE_DIFF_HPP

#include "pstore/core/hamt_set.hpp"

namespace pstore {

    namespace diff_details {

        template <typename Index>
        class traverser {
            using index_pointer = index::details::index_pointer;

        public:
            /// \param db  The owning database instance.
            /// \param index  The index to be traversed.
            /// \param threshold  Addresses less than the threshold value are "old".
            constexpr traverser (database const & db, Index const & index,
                                 address threshold) noexcept
                    : db_{db}
                    , index_{index}
                    , threshold_{threshold} {}

            /// \tparam OutputIterator An Output-Iterator type.
            /// \param out  The output iterator to which the address of leaves may be added.
            /// \result The output iterator to which results were written.
            template <typename OutputIterator>
            OutputIterator operator() (OutputIterator out) const;

        private:
            /// \tparam OutputIterator An Output-Iterator type.
            /// \param node  The index node to be visited.
            /// \param shifts  The depth of the node in the tree structure.
            /// \param out  The output iterator to which the address of leaves may be added.
            /// \result The output iterator to which results were written.
            template <typename OutputIterator>
            OutputIterator visit_node (index_pointer node, unsigned shifts,
                                       OutputIterator out) const;

            /// Recursively traverses the members of an internal or linear index node.
            ///
            /// \tparam Node An index node type: either internal_node or linear_node.
            /// \tparam OutputIterator An Output-Iterator type.
            /// \param node  The index node to be visited.
            /// \param shifts  The depth of the node in the tree structure.
            /// \param out  The output iterator to which the address of leaves may be added.
            /// \result The output iterator to which results were written.
            template <typename Node, typename OutputIterator>
            OutputIterator visit_intermediate (index_pointer node, unsigned shifts,
                                               OutputIterator out) const;

            bool is_new (index_pointer const node) const noexcept {
                return node.is_heap () ||
                       node.untag_internal_address ().to_address () >= threshold_;
            }

            database const & db_;
            Index const & index_;
            address const threshold_;
        };

        // operator()
        // ~~~~~~~~~~
        template <typename Index>
        template <typename OutputIterator>
        OutputIterator traverser<Index>::operator() (OutputIterator out) const {
            if (auto const root = index_.root ()) {
                out = this->visit_node (root, 0U, out);
            }
            return out;
        }

        // visit node
        // ~~~~~~~~~~
        template <typename Index>
        template <typename OutputIterator>
        OutputIterator traverser<Index>::visit_node (index_pointer node, unsigned shifts,
                                                     OutputIterator out) const {
            if (node.is_leaf ()) {
                assert (node.is_address ());
                // If this leaf is not in the "old" byte range then add it to the output
                // collection.
                if (this->is_new (node)) {
                    *out = node.addr;
                    ++out;
                }
                return out;
            }

            if (index::details::depth_is_internal_node (shifts)) {
                return this->visit_intermediate<index::details::internal_node> (node, shifts,
                                                                                out);
            }

            return this->visit_intermediate<index::details::linear_node> (node, shifts, out);
        }

        // visit intermediate
        // ~~~~~~~~~~~~~~~~~~
        template <typename Index>
        template <typename Node, typename OutputIterator>
        OutputIterator
        traverser<Index>::visit_intermediate (index::details::index_pointer const node,
                                              unsigned const shifts, OutputIterator out) const {
            std::pair<std::shared_ptr<void const>, Node const *> const p =
                Node::get_node (db_, node);
            assert (std::get<Node const *> (p) != nullptr);
            for (auto child : *std::get<Node const *> (p)) {
                if (this->is_new (node)) {
                    out = this->visit_node (index_pointer{child},
                                            shifts + index::details::hash_index_bits, out);
                }
            }
            return out;
        }

    } // end namespace diff_details


    /// Write a series of addresses to an output iterator of the objects that were added to an
    /// \p index between the current revision and the revision number given by \p old.
    ///
    /// \param db  The owning database instance.
    /// \param index  The index to be traversed.
    /// \param old  The revision number against which the index is to be compared.
    /// \param out  The output iterator to which the address of objects added to the index since
    ///   the given old revision.
    /// \result The output iterator to which results were written.
    template <typename Index, typename OutputIterator>
    OutputIterator diff (database const & db, Index const & index, revision_number const old,
                         OutputIterator out) {
        if (old == pstore::head_revision || old > db.get_current_revision ()) {
            return out;
        }
        // addresses less than the threshold value are "old".
        diff_details::traverser<Index> t{db, index, (db.older_revision_footer_pos (old) + 1).to_address ()};
        return t (out);
    }

} // end namespace pstore

#endif // PSTORE_CORE_DIFF_HPP
