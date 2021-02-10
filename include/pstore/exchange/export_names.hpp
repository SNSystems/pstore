//*                             _                                      *
//*   _____  ___ __   ___  _ __| |_   _ __   __ _ _ __ ___   ___  ___  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | '_ \ / _` | '_ ` _ \ / _ \/ __| *
//* |  __/>  <| |_) | (_) | |  | |_  | | | | (_| | | | | | |  __/\__ \ *
//*  \___/_/\_\ .__/ \___/|_|   \__| |_| |_|\__,_|_| |_| |_|\___||___/ *
//*           |_|                                                      *
//===- include/pstore/exchange/export_names.hpp ---------------------------===//
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
            /// \p os  The stream to which output is written.
            /// \p ind  The indentation of the output.
            /// \p db  The database instance whose strings are to be dumped.
            /// \p generation  The database generation number whose strings are to be dumped.
            /// \p string_table  The string table accumulates the address-to-index mapping of each
            /// string as it is dumped.
            void emit_names (ostream_base & os, indent ind, database const & db,
                             unsigned const generation, name_mapping * const string_table);

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_NAMES_HPP
