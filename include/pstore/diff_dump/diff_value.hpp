//*      _ _  __  __              _             *
//*   __| (_)/ _|/ _| __   ____ _| |_   _  ___  *
//*  / _` | | |_| |_  \ \ / / _` | | | | |/ _ \ *
//* | (_| | |  _|  _|  \ V / (_| | | |_| |  __/ *
//*  \__,_|_|_| |_|     \_/ \__,_|_|\__,_|\___| *
//*                                             *
//===- include/pstore/diff_dump/diff_value.hpp ----------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_DIFF_DUMP_DIFF_VALUE_HPP
#define PSTORE_DIFF_DUMP_DIFF_VALUE_HPP

#include "pstore/core/diff.hpp"
#include "pstore/diff_dump/revision.hpp"
#include "pstore/dump/db_value.hpp"

namespace pstore {
    namespace diff_dump {

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

    } // end namespace diff_dump
} // end namespace pstore

namespace pstore {
    namespace diff_dump {

        namespace details {

            /// A simple RAII helper class which saves and restores the current db revision number.
            class revision_restorer {
            public:
                explicit revision_restorer (database & db)
                        : db_{db}
                        , old_revision_{db.get_current_revision ()} {}
                revision_restorer (revision_restorer const &) = delete;

                ~revision_restorer () noexcept {
                    no_ex_escape ([this] () { db_.sync (old_revision_); });
                }

                revision_restorer & operator= (revision_restorer const &) = delete;

            private:
                database & db_;
                unsigned const old_revision_;
            };

            /// An OutputIterator which converts the address of objects in a known index into their
            /// value_ptr representation for display to the user.
            template <typename Index>
            class diff_out {
            public:
                using iterator_category = std::output_iterator_tag;
                using value_type = void;
                using difference_type = void;
                using pointer = void;
                using reference = void;

                struct params {
                    params (gsl::not_null<database const *> const db,
                            gsl::not_null<Index const *> const index,
                            gsl::not_null<dump::array::container *> const members) noexcept
                            : db_{db}
                            , index_{index}
                            , members_{members} {}

                    gsl::not_null<database const *> const db_;
                    gsl::not_null<Index const *> const index_;
                    gsl::not_null<dump::array::container *> const members_;
                };

                explicit diff_out (gsl::not_null<params *> const p) noexcept
                        : p_{p} {}

                diff_out & operator= (pstore::address addr) {
                    p_->members_->emplace_back (
                        dump::make_value (get_key (p_->index_->load_leaf_node (*p_->db_, addr))));
                    return *this;
                }

                diff_out & operator* () noexcept { return *this; }
                diff_out & operator++ () noexcept { return *this; }
                diff_out operator++ (int) noexcept { return *this; }

            private:
                gsl::not_null<params *> p_;
            };

        } // end namespace details

        /// Make a value pointer which contains the keys that are different between two database
        /// revisions.
        ///
        /// \param db The database from which the index is to be read.
        /// \param old_revision  A old database revision number to be compared to new_contents.
        /// \param get_index A function which is responsible for getting a specific index from the
        ///                  database. It should have the signature: Index * (database &, bool).
        /// \returns  A value pointer which contains all different keys between two revisions.
        template <typename Index, typename GetIndexFunction>
        dump::value_ptr make_diff (database & db, revision_number const old_revision,
                                   GetIndexFunction get_index) {
            dump::array::container members;
            std::shared_ptr<Index const> const index = get_index (db, true /* create */);
            typename details::diff_out<Index>::params params{&db, index.get (), &members};
            diff (db, *index, old_revision, details::diff_out<Index>{&params});
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
            PSTORE_ASSERT (new_revision >= old_revision);

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
        dump::value_ptr make_indices_diff (database & db, revision_number const new_revision,
                                           revision_number const old_revision);

    } // end namespace diff_dump
} // end namespace pstore

#endif // PSTORE_DIFF_DUMP_DIFF_VALUE_HPP
