//===- lib/exchange/export_names.cpp --------------------------------------===//
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
#include "pstore/exchange/export_names.hpp"

#include <cassert>
#include <limits>

#include "pstore/exchange/export_ostream.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            //*                                             _            *
            //*  _ _  __ _ _ __  ___   _ __  __ _ _ __ _ __(_)_ _  __ _  *
            //* | ' \/ _` | '  \/ -_) | '  \/ _` | '_ \ '_ \ | ' \/ _` | *
            //* |_||_\__,_|_|_|_\___| |_|_|_\__,_| .__/ .__/_|_||_\__, | *
            //*                                  |_|  |_|         |___/  *
            // (ctor)
            // ~~~~~~
            name_mapping::name_mapping (database const & db) {
                auto const index = index::get_index<trailer::indices::name> (db);
                names_.reserve (index->size ());
            }

            // add
            // ~~~
            void name_mapping::add (address const addr) {
                PSTORE_ASSERT (names_.find (addr) == names_.end ());
                PSTORE_ASSERT (names_.size () <= std::numeric_limits<std::uint64_t>::max ());
                auto const index = static_cast<std::uint64_t> (names_.size ());
                names_[addr] = index;
            }

            // index
            // ~~~~~
            std::uint64_t name_mapping::index (typed_address<indirect_string> const addr) const {
                auto const pos = names_.find (addr.to_address ());
                PSTORE_ASSERT (pos != names_.end ());
                return pos->second;
            }

            //*            _ _                             *
            //*  ___ _ __ (_) |_   _ _  __ _ _ __  ___ ___ *
            //* / -_) '  \| |  _| | ' \/ _` | '  \/ -_|_-< *
            //* \___|_|_|_|_|\__| |_||_\__,_|_|_|_\___/__/ *
            //*                                            *
            void emit_names (ostream_base & os, indent const ind, database const & db,
                             unsigned const generation, name_mapping * const string_table) {
                auto const names_index = index::get_index<trailer::indices::name> (db);
                PSTORE_ASSERT (generation > 0);

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
