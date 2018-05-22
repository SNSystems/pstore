//*      _ _  __  __              _             *
//*   __| (_)/ _|/ _| __   ____ _| |_   _  ___  *
//*  / _` | | |_| |_  \ \ / / _` | | | | |/ _ \ *
//* | (_| | |  _|  _|  \ V / (_| | | |_| |  __/ *
//*  \__,_|_|_| |_|     \_/ \__,_|_|\__,_|\___| *
//*                                             *
//===- include/pstore/diff/diff_value.hpp ---------------------------------===//
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
#ifndef PSTORE_DIFF_VALUE_HPP
#define PSTORE_DIFF_VALUE_HPP

#include <set>

#include "pstore/core/database.hpp"
#include "pstore/diff/revision.hpp"
#include "pstore/dump/db_value.hpp"

namespace pstore {
    namespace diff {

        /// Get the key from a given element of a container such as pstore::hamt_set.
        template <typename KeyType>
        inline KeyType get_key (KeyType const & v) {
            return v;
        }
        /// Get the key from a given element of an associative container such as pstore::hamt_map.
        template <typename KeyType, typename ValueType>
        inline KeyType get_key (std::pair<KeyType, ValueType> const & kvp) {
            return kvp.first;
        }

        /// Get the value (which is the same as the key) from a given a container such as
        /// pstore::hamt_set.
        template <typename KeyType>
        inline KeyType get_value (KeyType const & v) {
            return v;
        }
        /// Get the value from a given element of an associative container such as pstore::hamt_map.
        template <typename KeyType, typename ValueType>
        inline ValueType get_value (std::pair<KeyType, ValueType> const & kvp) {
            return kvp.second;
        }

        /// Build up a set of the index values by syncing the database to the given revision and
        /// getting the specific index using get_index().
        ///
        /// \param db The database from which the index is to be read.
        /// \param new_revision  A specified revision of the data to be updated.
        /// \param get_index A function pointer which could get specific index from the database.
        /// \returns  A set of index values which exist in the database at the given revision.
        template <typename Index, typename GetIndexFunction>
        std::set<typename Index::value_type>
        build_index_values (database & db, diff::revision_number const new_revision,
                            GetIndexFunction get_index) {
            std::set<typename Index::value_type> result;
            db.sync (new_revision);
            if (std::shared_ptr<Index const> const index = get_index (db, false /* create */)) {
                for (auto const & value : *index) {
                    result.insert (value);
                }
            }
            return result;
        }

    } // namespace diff
} // namespace pstore

namespace pstore {
    namespace diff {

        /// Make a value pointer which contains the keys that are different between two database
        /// revisions.
        ///
        /// \param new_contents  A set of index values when database at the new revision.
        /// \param db The database from which the index is to be read.
        /// \param old_revision  A old database revision number to be compared to new_contents.
        /// \param get_index A function which is responsible for getting a specific index from the
        ///                  database. It should have the signature: Index * (database &, bool).
        /// \returns  A value pointer which contains all different keys between two revisions.
        template <typename Index, typename GetIndexFunction>
        dump::value_ptr make_diff (std::set<typename Index::value_type> const & new_contents,
                                   database & db, diff::revision_number const old_revision,
                                   GetIndexFunction get_index) {
            db.sync (old_revision);
            dump::array::container members;
            std::shared_ptr<Index const> const index = get_index (db, true /* create */);
            auto const end = index->end ();
            for (auto const & member : new_contents) {
                auto const it = index->find (diff::get_key (member));
                if (it == end || get_value (*it) != get_value (member)) {
                    members.emplace_back (dump::make_value (get_key (member)));
                }
            }
            return dump::make_value (members);
        }

        /// Make a value pointer which contains all different keys between two revisions for a
        /// specific
        /// index.
        ///
        /// \pre new_revision >= old_revision
        ///
        /// \param name  The name of a specific index.
        /// \param db The database from which the index is to be read.
        /// \param new_revision  A new database revision number.
        /// \param old_revision  An old database revision number.
        /// \param get_index A function which is responsible for getting a specific index from the
        ///                  database. It should have the signature: Index * (database &, bool).
        /// \returns  A value pointer which contains a given index name and all different keys
        ///           between two revisions.
        template <typename Index, typename GetIndexFunction>
        dump::value_ptr
        make_index_diff (char const * name, database & db, revision_number const new_revision,
                         revision_number const old_revision, GetIndexFunction get_index) {
            assert (new_revision >= old_revision);
            std::set<typename Index::value_type> values =
                build_index_values<Index> (db, new_revision, get_index);

            using dump::make_value;
            return dump::make_value (dump::object::container{
                {"name", make_value (name)},
                {"members", make_diff<Index> (values, db, old_revision, get_index)},
            });
        }

        /// Make a value pointer which contains all different keys between two revisions for all
        /// database indices.
        ///
        /// \pre new_revision >= old_revision
        ///
        /// \param db The database from which the index is to be read.
        /// \param new_revision  A new database revision number.
        /// \param old_revision  An old database revision number.
        /// \returns  A value pointer which contains all different indices keys.
        dump::value_ptr make_indices_diff (database & db, diff::revision_number const new_revision,
                                           diff::revision_number const old_revision);

    } // namespace diff
} // namespace pstore

#endif // PSTORE_DIFF_VALUE_HPP
// eof: include/pstore/diff/diff_value.hpp
