//*                             _      __                                      _    *
//*   _____  ___ __   ___  _ __| |_   / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__| |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*           |_|                                   |___/                           *
//===- lib/exchange/export_fragment.cpp -----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/export_fragment.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            void emit_fragments (ostream & os, indent const ind, database const & db,
                                 unsigned const generation, name_mapping const & names,
                                 bool const comments) {
                auto const fragments = index::get_index<trailer::indices::fragment> (db);
                if (!fragments->empty ()) {
                    auto const * fragment_sep = "\n";
                    PSTORE_ASSERT (generation > 0U);

                    auto const out_fn = [&] (address addr) {
                        auto const & kvp = fragments->load_leaf_node (db, addr);
                        os << fragment_sep << ind;
                        emit_digest (os, kvp.first);
                        os << ':';
                        emit_fragment (os, ind.next (), db, names, db.getro (kvp.second), comments);
                        fragment_sep = ",\n";
                    };

                    diff (db, *fragments, generation - 1U, make_diff_out (&out_fn));
                }
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore
