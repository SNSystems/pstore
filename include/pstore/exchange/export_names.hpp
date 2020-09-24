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

#include <cstdint>
#include <unordered_map>

#include "pstore/core/address.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/indirect_string.hpp"
#include "pstore/diff/diff.hpp"
#include "pstore/exchange/export_emit.hpp"

namespace pstore {
    namespace exchange {

        class export_name_mapping {
        public:
            void add (address addr);

            std::uint64_t index (address addr) const;
            std::uint64_t index (typed_address<indirect_string> const addr) const {
                return this->index (addr.to_address ());
            }

        private:
            std::unordered_map<address, std::uint64_t> names_;
        };

        //*                        _                             *
        //*  _____ ___ __  ___ _ _| |_   _ _  __ _ _ __  ___ ___ *
        //* / -_) \ / '_ \/ _ \ '_|  _| | ' \/ _` | '  \/ -_|_-< *
        //* \___/_\_\ .__/\___/_|  \__| |_||_\__,_|_|_|_\___/__/ *
        //*         |_|                                          *
        template <typename OStream>
        void export_names (OStream & os, indent ind, database const & db, unsigned const generation,
                           export_name_mapping * const string_table) {

            auto names_index = index::get_index<trailer::indices::name> (db);
            assert (generation > 0);
            auto const container = diff::diff (db, *names_index, generation - 1U);
            emit_array (os, ind, std::begin (container), std::end (container),
                        [&names_index, &string_table, &db] (OStream & os1, indent ind1,
                                                            address const addr) {
                            indirect_string const str = names_index->load_leaf_node (db, addr);
                            shared_sstring_view owner;
                            raw_sstring_view view = str.as_db_string_view (&owner);
                            os1 << ind1;
                            emit_string (os1, view);
                            string_table->add (addr);
                        });
        }

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_NAMES_HPP
