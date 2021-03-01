//===- lib/exchange/export_compilation.cpp --------------------------------===//
//*                             _    *
//*   _____  ___ __   ___  _ __| |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| *
//* |  __/>  <| |_) | (_) | |  | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__| *
//*           |_|                    *
//*                            _ _       _   _              *
//*   ___ ___  _ __ ___  _ __ (_) | __ _| |_(_) ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ \| | |/ _` | __| |/ _ \| '_ \  *
//* | (_| (_) | | | | | | |_) | | | (_| | |_| | (_) | | | | *
//*  \___\___/|_| |_| |_| .__/|_|_|\__,_|\__|_|\___/|_| |_| *
//*                     |_|                                 *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file export_compilation.cpp
/// \brief  Functions for exporting compilations and the compilation index.
#include "pstore/exchange/export_compilation.hpp"

#include "pstore/exchange/export_ostream.hpp"

using pstore::exchange::export_ns::ostream_base;

namespace pstore {
    namespace repo {

#define X(a)                                                                                       \
    case repo::linkage::a: return os << #a;
        ostream_base & operator<< (ostream_base & os, linkage const l);
        ostream_base & operator<< (ostream_base & os, linkage const l) {
            switch (l) { PSTORE_REPO_LINKAGES }
            return os << "unknown";
        }
#undef X

        ostream_base & operator<< (ostream_base & os, visibility const v);
        ostream_base & operator<< (ostream_base & os, visibility const v) {
            switch (v) {
            case repo::visibility::default_vis: return os << "default";
            case repo::visibility::hidden_vis: return os << "hidden";
            case repo::visibility::protected_vis: return os << "protected";
            }
            return os << "unknown";
        }

    } // end namespace repo
} // end namespace pstore

namespace pstore {
    namespace exchange {
        namespace export_ns {

            void emit_compilation (ostream_base & os, indent const ind, database const & db,
                                   repo::compilation const & compilation,
                                   name_mapping const & names, bool const comments) {
                os << "{\n";
                auto const object_indent = ind.next ();
                os << object_indent << R"("path":)" << names.index (compilation.path ()) << ',';
                show_string (os, db, compilation.path (), comments);
                os << '\n'
                   << object_indent << R"("triple":)" << names.index (compilation.triple ()) << ',';
                show_string (os, db, compilation.triple (), comments);
                os << '\n' << object_indent << R"("definitions":)";
                emit_array_with_name (os, object_indent, db, compilation.begin (),
                                      compilation.end (), comments,
                                      [&] (ostream_base & os1, repo::definition const & d) {
                                          os1 << R"({"digest":)";
                                          emit_digest (os1, d.digest);
                                          os1 << R"(,"name":)" << names.index (d.name)
                                              << R"(,"linkage":")" << d.linkage () << '"';
                                          if (d.visibility () != repo::visibility::default_vis) {
                                              os1 << R"(,"visibility":")" << d.visibility () << '"';
                                          }
                                          os1 << '}';
                                          return d.name;
                                      });
                os << '\n' << ind << '}';
            }


            void emit_compilation_index (ostream_base & os, indent const ind, database const & db,
                                         unsigned const generation, name_mapping const & names,
                                         bool const comments) {
                auto const compilations = index::get_index<trailer::indices::compilation> (db);
                if (!compilations || compilations->empty ()) {
                    return;
                }
                if (generation == 0) {
                    // The first (zeroth) transaction in the store is, by definition, empty.
                    return;
                }
                auto const * sep = "\n";

                auto const out_fn = [&] (address const addr) {
                    auto const & kvp = compilations->load_leaf_node (db, addr);
                    os << sep << ind;
                    emit_digest (os, kvp.first);
                    os << ':';
                    emit_compilation (os, ind, db, *db.getro (kvp.second), names, comments);
                    sep = ",\n";
                };
                diff (db, *compilations, generation - 1U, make_diff_out (&out_fn));
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore
