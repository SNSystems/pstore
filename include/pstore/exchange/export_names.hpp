//*                             _                                      *
//*   _____  ___ __   ___  _ __| |_   _ __   __ _ _ __ ___   ___  ___  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | '_ \ / _` | '_ ` _ \ / _ \/ __| *
//* |  __/>  <| |_) | (_) | |  | |_  | | | | (_| | | | | | |  __/\__ \ *
//*  \___/_/\_\ .__/ \___/|_|   \__| |_| |_|\__,_|_| |_| |_|\___||___/ *
//*           |_|                                                      *
//===- include/pstore/exchange/export_names.hpp ---------------------------===//
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
#ifndef PSTORE_EXCHANGE_EXPORT_NAMES_HPP
#define PSTORE_EXCHANGE_EXPORT_NAMES_HPP

#include <unordered_map>

#include "pstore/core/diff.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/exchange/export_emit.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

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
                explicit name_mapping (database const & db);

                /// Record the address of a string at \p addr and assign it the next index in in the
                /// exported names array.
                ///
                /// \param addr The address of a string beging exported.
                void add (address addr);

                /// Convert the address of the string at \p addr to the corresponding index in the
                /// exported names array.
                ///
                /// \param addr  The address of a string previously passed to the add() method.
                /// \result The index of the string at \p addr in the exported names array.
                std::uint64_t index (typed_address<indirect_string> const addr) const;

            private:
                std::unordered_map<address, std::uint64_t> names_;
            };

            //*            _ _                             *
            //*  ___ _ __ (_) |_   _ _  __ _ _ __  ___ ___ *
            //* / -_) '  \| |  _| | ' \/ _` | '  \/ -_|_-< *
            //* \___|_|_|_|_|\__| |_||_\__,_|_|_|_\___/__/ *
            //*                                            *
            /// Writes the array of names defined in transaction \p generation to output stream
            /// \p os.
            ///
            /// \tparam OStream An iostreams-style output type.
            /// \p os  The stream to which output is written.
            /// \p ind  The indentation of the output.
            /// \p db  The database instance whose strings are to be dumped.
            /// \p generation  The database generation number whose strings are to be dumped.
            /// \p string_table  The string table accumulates the address-to-index mapping of each
            /// string as it is dumped.
            template <typename OStream>
            void emit_names (OStream & os, indent ind, database const & db,
                             unsigned const generation, name_mapping * const string_table) {
                auto const names_index = index::get_index<trailer::indices::name> (db);
                assert (generation > 0);

                auto const * separator = "";
                auto const * tail_separator = "";
                auto close_bracket_indent = indent{};

                auto member_indent = ind.next ();
                auto const out_fn = [&] (pstore::address const addr) {
                    os << separator << '\n' << member_indent;
                    {
                        indirect_string const str = names_index->load_leaf_node (db, addr);
                        shared_sstring_view owner;
                        raw_sstring_view view = str.as_db_string_view (&owner);
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
