//*  _           _             _                          *
//* (_)_ __   __| | _____  __ | |_ _   _ _ __   ___  ___  *
//* | | '_ \ / _` |/ _ \ \/ / | __| | | | '_ \ / _ \/ __| *
//* | | | | | (_| |  __/>  <  | |_| |_| | |_) |  __/\__ \ *
//* |_|_| |_|\__,_|\___/_/\_\  \__|\__, | .__/ \___||___/ *
//*                                |___/|_|               *
//===- lib/core/index_types.cpp -------------------------------------------===//
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

#include "pstore/core/index_types.hpp"

#include "pstore/core/hamt_set.hpp"

namespace {

    constexpr auto index_integral (pstore::trailer::indices const idx) noexcept
        -> std::underlying_type<pstore::trailer::indices>::type {
        return static_cast<std::underlying_type<pstore::trailer::indices>::type> (idx);
    }

    template <pstore::trailer::indices Index>
    void flush_index (pstore::transaction_base & transaction,
                      pstore::trailer::index_records_array * const locations,
                      unsigned const generation) {
        pstore::database & db = transaction.db ();
        if (auto const index = pstore::index::get_index<Index> (db, false /*create*/)) {
            (*locations)[index_integral (Index)] = index->flush (transaction, generation);
        }
    }

} // end anonymous namespace

namespace pstore {
    namespace index {

        // flush_indices
        // ~~~~~~~~~~~~~
        void flush_indices (transaction_base & transaction,
                            trailer::index_records_array * const locations,
                            unsigned const generation) {
#define X(k)                                                                                       \
    case trailer::indices::k:                                                                      \
        flush_index<trailer::indices::k> (transaction, locations, generation);                     \
        break;

            for (auto ctr = std::underlying_type<trailer::indices>::type{0};
                 ctr <= index_integral (trailer::indices::last); ++ctr) {
                switch (static_cast<trailer::indices> (ctr)) {
                    PSTORE_INDICES
                case trailer::indices::last: break;
                }
            }
#undef X
            assert (locations->size () == index_integral (trailer::indices::last));
        }

    } // end namespace index
} // end namespace pstore
