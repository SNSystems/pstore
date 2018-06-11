//*      _ _  __  __  *
//*   __| (_)/ _|/ _| *
//*  / _` | | |_| |_  *
//* | (_| | |  _|  _| *
//*  \__,_|_|_| |_|   *
//*                   *
//===- include/pstore/diff/diff.hpp ---------------------------------------===//
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
#ifndef PSTORE_DIFF_DIFF_HPP
#define PSTORE_DIFF_DIFF_HPP

#include <deque>

#include "pstore/config/config.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/hamt_map_types.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/diff/revision.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/portab.hpp"

namespace pstore {
    namespace diff {

        using result_type = std::deque<address>;

        namespace details {

            using index::details::index_pointer;

            template <typename Index>
            class traverser {
            public:
                /// \param index  The index to be traversed.
                /// \param threshold  Addresses less than the threshold value are "old".
                traverser (Index const & index, address threshold) noexcept
                        : index_{index}
                        , threshold_{threshold} {}

                result_type operator() () const;

            private:
                /// \param node  The index node to be visited.
                /// \param shifts  The depth of the node in the tree structure.
                /// \param result  A container to which the address of leaf nodes may be added.
                void visit_node (index_pointer node, unsigned shifts,
                                 gsl::not_null<result_type *> result) const;

                /// Recursively traverses the members of an internal or linear index node.
                ///
                /// \param node  The index node to be visited.
                /// \param shifts  The depth of the node in the tree structure.
                /// \param result  A container to which the address of leaf nodes may be added.
                template <typename Node>
                void visit_intermediate (index_pointer node, unsigned shifts,
                                         gsl::not_null<result_type *> result) const;

                bool is_new (index_pointer node) const noexcept {
                    return node.is_heap () || node.untag_internal_address () >= threshold_;
                }

                Index const & index_;
                address const threshold_;
            };

            // operator()
            // ~~~~~~~~~~
            template <typename Index>
            result_type traverser<Index>::operator() () const {
                result_type result;
                if (auto const root = index_.root ()) {
                    this->visit_node (root, 0U, &result);
                }
                return result;
            }

            // visit_node
            // ~~~~~~~~~~
            template <typename Index>
            void traverser<Index>::visit_node (index_pointer node, unsigned shifts,
                                               gsl::not_null<result_type *> result) const {
                if (node.is_leaf ()) {
                    assert (node.is_address ());
                    // If this leaf is not in the "old" byte range then add it to the output
                    // collection.
                    if (this->is_new (node)) {
                        result->push_back (node.addr);
                    }
                } else if (index::details::depth_is_internal_node (shifts)) {
                    this->visit_intermediate<index::details::internal_node> (node, shifts, result);
                } else {
                    this->visit_intermediate<index::details::linear_node> (node, shifts, result);
                }
            }

            // visit_intermediate
            // ~~~~~~~~~~~~~~~~~~
            template <typename Index>
            template <typename Node>
            void traverser<Index>::visit_intermediate (index::details::index_pointer node,
                                                       unsigned shifts,
                                                       gsl::not_null<result_type *> result) const {
                std::shared_ptr<void const> store_ptr;
                Node const * ptr = nullptr;
                std::tie (store_ptr, ptr) = Node::get_node (index_.db (), node);
                assert (ptr != nullptr);

                for (index_pointer child : *ptr) {
                    if (this->is_new (node)) {
                        this->visit_node (child, shifts + index::details::hash_index_bits, result);
                    }
                }
            }

        } // end namespace details


        template <typename Index>
        result_type diff (Index const & index, revision_number old) {
            auto & db = index.db ();
            if (old == pstore::head_revision || old > db.get_current_revision ()) {
                return {};
            }
            // addresses less than the threshold value are "old".
            details::traverser<Index> t{index,
                                        (db.older_revision_footer_pos (old) + 1).to_address ()};
            return t ();
        }

    } // end namespace diff
} // end namespace pstore

#endif // PSTORE_DIFF_DIFF_HPP
// eof: include/pstore/diff/diff.hpp
