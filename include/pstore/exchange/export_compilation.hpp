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

#include "pstore/exchange/export_emit.hpp"
#include "pstore/exchange/export_names.hpp"
#include "pstore/mcrepo/compilation.hpp"

namespace pstore {
    namespace exchange {

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
        void export_compilation (OStream & os, database const & db,
                                 pstore::repo::compilation const & compilation,
                                 export_name_mapping const & names) {
            os << "{\n" << indent5 << R"("path":)" << names.index (compilation.path ()) << ',';
            show_string (os, db, compilation.path ());
            os << '\n' << indent5 << R"("triple":)" << names.index (compilation.triple ()) << ',';
            show_string (os, db, compilation.triple ());
            os << '\n' << indent5 << R"("definitions":)";
            emit_array (os, compilation.begin (), compilation.end (), indent5,
                        [&db, &names] (OStream & os1, repo::compilation_member const & d) {
                            os1 << indent6 << "{\n";
                            os1 << indent7 << R"("digest":")" << d.digest.to_hex_string ()
                                << "\",\n";
                            os1 << indent7 << R"("name":)" << names.index (d.name) << ',';
                            show_string (os1, db, d.name);
                            os1 << '\n';
                            os1 << indent7 << R"("linkage":")" << d.linkage () << "\",\n";
                            os1 << indent7 << R"("visibility":")" << d.visibility () << "\"\n";
                            os1 << indent6 << '}';
                        });
            os << '\n' << indent4 << '}';
        }


        template <typename OStream>
        void export_compilation_index (OStream & os, database const & db, unsigned const generation,
                                       export_name_mapping const & names) {
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
                os << sep << indent4 << '\"' << kvp.first.to_hex_string () << "\": ";
                export_compilation (os, db, *db.getro (kvp.second), names);
                sep = ",\n";
            }
        }

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_COMPILATION_HPP
