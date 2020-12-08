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
//===- include/pstore/exchange/export_compilation.hpp ---------------------===//
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
#ifndef PSTORE_EXCHANGE_EXPORT_COMPILATION_HPP
#define PSTORE_EXCHANGE_EXPORT_COMPILATION_HPP

#include "pstore/exchange/export_names.hpp"
#include "pstore/mcrepo/compilation.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

#define X(a)                                                                                       \
case repo::linkage::a: return os << #a;
            template <typename OStream>
            OStream & operator<< (OStream & os, repo::linkage const linkage) {
                switch (linkage) { PSTORE_REPO_LINKAGES }
                return os << "unknown";
            }
#undef X

            template <typename OStream>
            OStream & operator<< (OStream & os, repo::visibility const visibility) {
                switch (visibility) {
                case repo::visibility::default_vis: return os << "default";
                case repo::visibility::hidden_vis: return os << "hidden";
                case repo::visibility::protected_vis: return os << "protected";
                }
                return os << "unknown";
            }


            template <typename OStream>
            void emit_compilation (OStream & os, indent const ind, database const & db,
                                   repo::compilation const & compilation,
                                   name_mapping const & names, bool comments) {
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
                                      [&] (OStream & os1, repo::definition const & d) {
                                          os1 << R"({"digest":")" << d.digest.to_hex_string ()
                                              << R"(","name":)" << names.index (d.name)
                                              << R"(,"linkage":")" << d.linkage () << '"';
                                          if (d.visibility () != repo::visibility::default_vis) {
                                              os1 << R"(,"visibility":")" << d.visibility () << '"';
                                          }
                                          os1 << '}';
                                          return d.name;
                                      });
                os << '\n' << ind << '}';
            }


            template <typename OStream>
            void emit_compilation_index (OStream & os, indent const ind, database const & db,
                                         unsigned const generation, name_mapping const & names,
                                         bool comments) {
                auto const compilations = index::get_index<trailer::indices::compilation> (db);
                if (!compilations || compilations->empty ()) {
                    return;
                }
                if (generation == 0) {
                    // The first (zeroth) transaction in the store is, by definition, empty.
                    return;
                }
                auto const * sep = "\n";
                for (address const & addr : diff::diff (db, *compilations, generation - 1U)) {
                    auto const & kvp = compilations->load_leaf_node (db, addr);
                    os << sep << ind << '\"' << kvp.first.to_hex_string () << R"(":)";
                    emit_compilation (os, ind, db, *db.getro (kvp.second), names, comments);
                    sep = ",\n";
                }
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_COMPILATION_HPP
