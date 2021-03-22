//===- include/pstore/exchange/export_names.hpp -----------*- mode: C++ -*-===//
//*                             _                                      *
//*   _____  ___ __   ___  _ __| |_   _ __   __ _ _ __ ___   ___  ___  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | '_ \ / _` | '_ ` _ \ / _ \/ __| *
//* |  __/>  <| |_) | (_) | |  | |_  | | | | (_| | | | | | |  __/\__ \ *
//*  \___/_/\_\ .__/ \___/|_|   \__| |_| |_|\__,_|_| |_| |_|\___||___/ *
//*           |_|                                                      *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_EXPORT_NAMES_HPP
#define PSTORE_EXCHANGE_EXPORT_NAMES_HPP

#include <unordered_map>

#include "pstore/core/diff.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/exchange/export_emit.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            class ostream_base;

            template <typename trailer::indices Index>
            struct index_tag_ {
                static constexpr auto index = Index;
            };

            inline constexpr decltype (auto) name_index_tag () noexcept {
                return index_tag_<pstore::trailer::indices::name>{};
            }
            inline constexpr decltype (auto) path_index_tag () noexcept {
                return index_tag_<pstore::trailer::indices::path>{};
            }


            //*                                             _            *
            //*  _ _  __ _ _ __  ___   _ __  __ _ _ __ _ __(_)_ _  __ _  *
            //* | ' \/ _` | '  \/ -_) | '  \/ _` | '_ \ '_ \ | ' \/ _` | *
            //* |_||_\__,_|_|_|_\___| |_|_|_\__,_| .__/ .__/_|_||_\__, | *
            //*                                  |_|  |_|         |___/  *
            /// The name_mapping call is used to associate the addresses of a set of strings with an
            /// index in the exported names array. The enables the exported JSON to simply reference
            /// a string by index rather than having to emit the string each time.
            class name_mapping {
            public:
                template <typename trailer::indices Index>
                name_mapping (database const & db, index_tag_<Index>) {
                    auto const index = index::get_index<Index> (db);
                    names_.reserve (index->size ());
                }

                /// Record the address of a string at \p addr and assign it the next index in in the
                /// exported names array.
                ///
                /// \param addr The address of a string being exported.
                void add (address addr);

                /// Returns the number of known addresss to index mappings.
                std::size_t size () const noexcept { return names_.size (); }

                /// Convert the address of the string at \p addr to the corresponding index in the
                /// exported names array.
                ///
                /// \param addr  The address of a string previously passed to the add() method.
                /// \result The index of the string at \p addr in the exported names array.
                std::uint64_t index (typed_address<indirect_string> const addr) const;

            private:
                std::unordered_map<address, std::uint64_t> names_;
            };

            //*            _ _        _       _               *
            //*  ___ _ __ (_) |_   __| |_ _ _(_)_ _  __ _ ___ *
            //* / -_) '  \| |  _| (_-<  _| '_| | ' \/ _` (_-< *
            //* \___|_|_|_|_|\__| /__/\__|_| |_|_||_\__, /__/ *
            //*                                     |___/     *
            /// Writes the array of strings added to the index given by \p Index in transaction \p
            /// generation to the output stream \p os.
            ///
            /// \tparam Index  The index in which the strings are defined.
            /// \p os  The stream to which output is written.
            /// \p ind  The indentation of the output.
            /// \p db  The database instance whose strings are to be dumped.
            /// \p generation  The database generation number whose strings are to be dumped.
            /// \p string_table  The string table accumulates the address-to-index mapping of each
            ///   string as it is dumped.
            template <typename trailer::indices Index>
            void emit_strings (ostream_base & os, indent const ind, database const & db,
                               unsigned const generation, name_mapping * const string_table) {
                if (generation == 0U) {
                    os << "[]";
                    return;
                }
                auto const names_index = index::get_index<Index> (db, false /*create*/);
                if (names_index == nullptr) {
                    os << "[]";
                    return;
                }

                auto const * separator = "";
                auto const * tail_separator = "";
                auto close_bracket_indent = indent{};

                auto const member_indent = ind.next ();
                auto const out_fn = [&] (pstore::address const addr) {
                    os << separator << '\n' << member_indent;
                    {
                        indirect_string const str = names_index->load_leaf_node (db, addr);
                        shared_sstring_view owner;
                        raw_sstring_view const view = str.as_db_string_view (&owner);
                        emit_string (os, view);
                        string_table->add (addr);
                    }
                    separator = ",";
                    tail_separator = "\n";
                    close_bracket_indent = ind;
                };
                os << '[';
                diff (db, *names_index, generation - 1U, make_diff_out (&out_fn));
                os << tail_separator << close_bracket_indent << ']';
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_NAMES_HPP
