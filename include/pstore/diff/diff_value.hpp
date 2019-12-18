//*      _ _  __  __              _             *
//*   __| (_)/ _|/ _| __   ____ _| |_   _  ___  *
//*  / _` | | |_| |_  \ \ / / _` | | | | |/ _ \ *
//* | (_| | |  _|  _|  \ V / (_| | | |_| |  __/ *
//*  \__,_|_|_| |_|     \_/ \__,_|_|\__,_|\___| *
//*                                             *
//===- include/pstore/diff/diff_value.hpp ---------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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

#include "pstore/core/database.hpp"
#include "pstore/diff/diff.hpp"
#include "pstore/diff/revision.hpp"
#include "pstore/dump/db_value.hpp"
#include "pstore/support/portab.hpp"

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

    } // namespace diff
} // namespace pstore

namespace pstore {
    namespace diff {

        namespace details {
            /// A simple RAII helper class which saves and restores the current db revision number.
            class revision_restorer {
            public:
                explicit revision_restorer (database & db)
                        : db_{db}
                        , old_revision_{db.get_current_revision ()} {
                }
                revision_restorer (revision_restorer const & ) = delete;
                revision_restorer & operator= (revision_restorer const & ) = delete;
                ~revision_restorer () noexcept {
                    PSTORE_NO_EX_ESCAPE(db_.sync (old_revision_));
                }

            private:
                database & db_;
                unsigned old_revision_;
            };

        } // namespace details

        /// Make a value pointer which contains the keys that are different between two database
        /// revisions.
        ///
        /// \param db The database from which the index is to be read.
        /// \param old_revision  A old database revision number to be compared to new_contents.
        /// \param get_index A function which is responsible for getting a specific index from the
        ///                  database. It should have the signature: Index * (database &, bool).
        /// \returns  A value pointer which contains all different keys between two revisions.
        template <typename Index, typename GetIndexFunction>
        dump::value_ptr make_diff (database & db, diff::revision_number const old_revision,
                                   GetIndexFunction get_index) {
            dump::array::container members;
            std::shared_ptr<Index const> const index = get_index (db, true /* create */);

            auto const differences = diff (db, *index, old_revision);
            std::transform (std::begin (differences), std::end (differences),
                            std::back_inserter (members), [&db, &index](pstore::address addr) {
                                return dump::make_value (
                                    get_key (index->load_leaf_node (db, addr)));
                            });
            return dump::make_value (members);
        }

        /// Make a value pointer which contains all different keys between two revisions for a
        /// specific index.
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
        dump::value_ptr make_index_diff (gsl::czstring const name, database & db,
                                         revision_number const new_revision,
                                         revision_number const old_revision,
                                         GetIndexFunction get_index) {
            assert (new_revision >= old_revision);

            details::revision_restorer const _{db};
            db.sync (new_revision);

            return dump::make_value (dump::object::container{
                {"name", dump::make_value (name)},
                {"members", make_diff<Index> (db, old_revision, get_index)},
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
