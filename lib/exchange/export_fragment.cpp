//===- lib/exchange/export_fragment.cpp -----------------------------------===//
//*                             _      __                                      _    *
//*   _____  ___ __   ___  _ __| |_   / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__| |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*           |_|                                   |___/                           *
//===----------------------------------------------------------------------===//
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

            void emit_fragment (ostream_base & os, indent const ind, class database const & db,
                                string_mapping const & strings,
                                std::shared_ptr<repo::fragment const> const & fragment,
                                bool const comments) {
                os << "{\n";
                auto const * section_sep = "";
                for (repo::section_kind const section : *fragment) {
                    auto const object_indent = ind.next ();
                    os << section_sep << object_indent << '"' << emit_section_name (section)
                       << R"(":)";
#define X(a)                                                                                       \
    case repo::section_kind::a:                                                                    \
        emit_section<repo::section_kind::a> (os, object_indent, db, strings,                       \
                                             fragment->at<pstore::repo::section_kind::a> (),       \
                                             comments);                                            \
        break;
                    switch (section) {
                        PSTORE_MCREPO_SECTION_KINDS
                    case repo::section_kind::last:
                        // unreachable...
                        PSTORE_ASSERT (false);
                        break;
                    }
#undef X
                    section_sep = ",\n";
                }
                os << '\n' << ind << '}';
            }

            void emit_fragments (ostream & os, indent const ind, database const & db,
                                 unsigned const generation, string_mapping const & strings,
                                 bool const comments) {
                auto const fragments = index::get_index<trailer::indices::fragment> (db);
                if (!fragments->empty ()) {
                    auto const * fragment_sep = "\n";
                    PSTORE_ASSERT (generation > 0U);

                    auto const out_fn = [&] (address const addr) {
                        auto const & kvp = fragments->load_leaf_node (db, addr);
                        os << fragment_sep << ind;
                        emit_digest (os, kvp.first);
                        os << ':';
                        emit_fragment (os, ind, db, strings, db.getro (kvp.second), comments);
                        fragment_sep = ",\n";
                    };

                    diff (db, *fragments, generation - 1U, make_diff_out (&out_fn));
                }
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore
