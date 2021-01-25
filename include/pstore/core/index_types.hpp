//*  _           _             _                          *
//* (_)_ __   __| | _____  __ | |_ _   _ _ __   ___  ___  *
//* | | '_ \ / _` |/ _ \ \/ / | __| | | | '_ \ / _ \/ __| *
//* | | | | | (_| |  __/>  <  | |_| |_| | |_) |  __/\__ \ *
//* |_|_| |_|\__,_|\___/_/\_\  \__|\__, | .__/ \___||___/ *
//*                                |___/|_|               *
//===- include/pstore/core/index_types.hpp --------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef PSTORE_CORE_INDEX_TYPES_HPP
#define PSTORE_CORE_INDEX_TYPES_HPP

#include "pstore/core/indirect_string.hpp"

namespace pstore {
    namespace index {

        using digest = uint128;
        struct u128_hash {
            std::uint64_t operator() (digest const & v) const { return v.high (); }
        };

    } // namespace index

    namespace serialize {
        /// \brief A serializer for uint128
        template <>
        struct serializer<uint128> {

            /// \brief Writes an individual uint128 instance to an archive.
            ///
            /// \param archive  The archive to which the span will be written.
            /// \param v        The object value which is to be written.
            template <typename Archive>
            static auto write (Archive && archive, uint128 const & v)
                -> archive_result_type<Archive> {
                return archive.put (v);
            }

            /// \brief Writes a span of uint128 instances to an archive.
            ///
            /// \param archive  The archive to which the span will be written.
            /// \param span     The span which is to be written.
            template <typename Archive, typename SpanType>
            static auto writen (Archive && archive, SpanType span) -> archive_result_type<Archive> {
                static_assert (std::is_same<typename SpanType::element_type, uint128>::value,
                               "span type does not match the serializer type");
                return archive.putn (span);
            }

            /// \brief Reads a uint128 value from an archive.
            ///
            /// \param archive  The archive from which the value will be read.
            /// \param out      A reference to uninitialized memory into which a uint128 will be
            /// read.
            template <typename Archive>
            static void read (Archive && archive, uint128 & out) {
                PSTORE_ASSERT (reinterpret_cast<std::uintptr_t> (&out) % alignof (uint128) == 0);
                archive.get (out);
            }

            /// \brief Reads a span of uint128 values from an archive.
            ///
            /// \param archive  The archive from which the value will be read.
            /// \param span     A span pointing to uninitialized memory
            template <typename Archive, typename Span>
            static void readn (Archive && archive, Span span) {
                static_assert (std::is_same<typename Span::element_type, uint128>::value,
                               "span type does not match the serializer type");
                details::getn_helper::getn (std::forward<Archive> (archive), span);
            }
        };

    } // namespace serialize

    class database;
    class transaction_base;

    namespace repo {
        class fragment;
        class compilation;
    } // end namespace repo

    namespace index {

        using compilation_index = hamt_map<digest, extent<repo::compilation>, u128_hash>;
        using debug_line_header_index = hamt_map<digest, extent<std::uint8_t>, u128_hash>;
        using fragment_index = hamt_map<digest, extent<repo::fragment>, u128_hash>;
        using write_index = hamt_map<std::string, extent<char>>;

        struct fnv_64a_hash_indirect_string {
            std::uint64_t operator() (indirect_string const & indir) const {
                shared_sstring_view owner;
                return fnv_64a_hash () (indir.as_string_view (&owner));
            }
        };

        using name_index = hamt_set<indirect_string, fnv_64a_hash_indirect_string>;

        // clang-format off
        /// Maps from the indices kind enumeration to the type that is used to represent a database index of that kind.
        template <trailer::indices T>
        struct enum_to_index {};

        template <> struct enum_to_index<trailer::indices::compilation         > { using type = compilation_index; };
        template <> struct enum_to_index<trailer::indices::debug_line_header   > { using type = debug_line_header_index; };
        template <> struct enum_to_index<trailer::indices::fragment            > { using type = fragment_index; };
        template <> struct enum_to_index<trailer::indices::name                > { using type = name_index; };
        template <> struct enum_to_index<trailer::indices::write               > { using type = write_index; };
        // clang-format on

        /// Returns a pointer to a index, loading it from the store on first access. If 'create' is
        /// false and the index does not already exist then nullptr is returned.
        template <pstore::trailer::indices Index, typename Database = pstore::database,
                  typename Return =
                      typename inherit_const<Database, typename enum_to_index<Index>::type>::type>
        std::shared_ptr<Return> get_index (Database & db, bool const create = true) {
            auto & dx = db.get_mutable_index (Index);

            // Have we already loaded this index?
            if (dx.get () == nullptr) {
                std::shared_ptr<trailer const> const footer = db.get_footer ();
                typed_address<index::header_block> const location = footer->a.index_records.at (
                    static_cast<typename std::underlying_type<decltype (Index)>::type> (Index));
                if (location == decltype (location)::null ()) {
                    if (create) {
                        // Create a new (empty) index.
                        dx = std::make_shared<typename std::remove_const<Return>::type> (db);
                    }
                } else {
                    // Construct the index from the location.
                    dx = std::make_shared<typename std::remove_const<Return>::type> (db, location);
                }
            }

#ifdef PSTORE_CPP_RTTI
            PSTORE_ASSERT ((!create && dx.get () == nullptr) ||
                           dynamic_cast<Return *> (dx.get ()) != nullptr);
#endif
            return std::static_pointer_cast<Return> (dx);
        }

        /// Write out any indices that have changed. Any that haven't will
        /// continue to point at their previous incarnation. Update the
        /// members of the 'locations' array.
        ///
        /// This happens early in the process of commiting a transaction; we're
        /// allocating and writing space in the store here.
        ///
        /// \param transaction An open transaction to which the index data will be added.
        /// \param locations  An array of the index locations. Any modified indices
        ///                   will be modified to point at the new file address.
        /// \param generation  A revision number into which the index will be flushed.
        void flush_indices (::pstore::transaction_base & transaction,
                            trailer::index_records_array * const locations, unsigned generation);

    } // namespace index
} // namespace pstore
#endif // PSTORE_CORE_INDEX_TYPES_HPP
