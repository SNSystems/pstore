//*                             _    *
//*   _____  ___ __   ___  _ __| |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| *
//* |  __/>  <| |_) | (_) | |  | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__| *
//*           |_|                    *
//===- lib/exchange/export.cpp --------------------------------------------===//
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
#include "pstore/exchange/export.hpp"

#include <type_traits>

#include "pstore/adt/sstring_view.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/generation_iterator.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/indirect_string.hpp"
#include "pstore/diff/diff.hpp"
#include "pstore/exchange/export_emit.hpp"
#include "pstore/exchange/export_fragment.hpp"
#include "pstore/exchange/export_names.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/mcrepo/generic_section.hpp"
#include "pstore/mcrepo/section.hpp"
#include "pstore/support/base64.hpp"
#include "pstore/support/gsl.hpp"

using namespace pstore::exchange;

namespace {


    decltype (auto) footers (pstore::database const & db) {
        auto const num_transactions = db.get_current_revision () + 1;
        std::vector<pstore::typed_address<pstore::trailer>> footers;
        footers.reserve (num_transactions);

        pstore::generation_container transactions{db};
        std::copy (std::begin (transactions), std::end (transactions),
                   std::back_inserter (footers));
        assert (footers.size () == num_transactions);
        std::reverse (std::begin (footers), std::end (footers));
        return footers;
    }




#define X(a)                                                                                       \
    case pstore::repo::linkage::a: return os << #a;
    export_ostream & operator<< (export_ostream & os, pstore::repo::linkage const linkage) {
        switch (linkage) { PSTORE_REPO_LINKAGES }
        return os << "unknown";
    }
#undef X

    export_ostream & operator<< (export_ostream & os, pstore::repo::visibility const visibility) {
        switch (visibility) {
        case pstore::repo::visibility::default_vis: return os << "default";
        case pstore::repo::visibility::hidden_vis: return os << "hidden";
        case pstore::repo::visibility::protected_vis: return os << "protected";
        }
        return os << "unknown";
    }

    void compilations (export_ostream & os, pstore::database const & db, unsigned const generation,
                       export_name_mapping const & names) {
        auto const compilations =
            pstore::index::get_index<pstore::trailer::indices::compilation> (db);
        if (!compilations->empty ()) {
            auto const * sep = "\n";
            assert (generation > 0);
            for (pstore::address const & addr :
                 pstore::diff::diff (db, *compilations, generation - 1U)) {
                auto const & kvp = compilations->load_leaf_node (db, addr);
                auto const compilation = db.getro (kvp.second);

                os << sep << indent4 << '\"' << kvp.first.to_hex_string () << "\": {";
                os << '\n' << indent5 << "\"path\": " << names.index (compilation->path ()) << ',';
                show_string (os, db, compilation->path ());
                os << '\n'
                   << indent5 << "\"triple\": " << names.index (compilation->triple ()) << ',';
                show_string (os, db, compilation->triple ());
                os << '\n' << indent5 << "\"definitions\": ";
                emit_array (os, compilation->begin (), compilation->end (), indent5,
                            [&db, &names] (export_ostream & os1,
                                           pstore::repo::compilation_member const & d) {
                                os1 << indent6 << "{\n";
                                os1 << indent7 << R"("digest": ")" << d.digest.to_hex_string ()
                                    << "\",\n";
                                os1 << indent7 << "\"name\": " << names.index (d.name) << ',';
                                show_string (os1, db, d.name);
                                os1 << '\n';
                                os1 << indent7 << R"("linkage": ")" << d.linkage () << "\",\n";
                                os1 << indent7 << R"("visibility": ")" << d.visibility () << "\"\n";
                                os1 << indent6 << '}';
                            });
                os << '\n' << indent4 << '}';
                sep = ",\n";
            }
        }
    }

    void debug_line (export_ostream & os, pstore::database const & db, unsigned const generation) {
        auto const debug_line_headers =
            pstore::index::get_index<pstore::trailer::indices::debug_line_header> (db);
        if (!debug_line_headers->empty ()) {
            auto const * sep = "\n";
            assert (generation > 0);
            for (pstore::address const & addr :
                 pstore::diff::diff (db, *debug_line_headers, generation - 1U)) {
                auto const & kvp = debug_line_headers->load_leaf_node (db, addr);
                os << sep << indent4 << '"' << kvp.first.to_hex_string () << "\": \"";
                std::shared_ptr<std::uint8_t const> const data = db.getro (kvp.second);
                auto const * const ptr = data.get ();
                pstore::to_base64 (ptr, ptr + kvp.second.size, ostream_inserter{os});
                os << '"';

                sep = ",\n";
            }
        }
    }

} // end anonymous namespace

namespace pstore {
    namespace exchange {

        void export_database (database & db, export_ostream & os) {
            export_name_mapping string_table;
            os << "{\n";
            os << indent1 << "\"version\": 1,\n";
            os << indent1 << "\"transactions\": ";

            auto const f = footers (db);
            assert (std::distance (std::begin (f), std::end (f)) >= 1);
            emit_array (
                os, std::next (std::begin (f)), std::end (f), indent1,
                [&db, &string_table] (export_ostream & os1,
                                      pstore::typed_address<pstore::trailer> const footer_pos) {
                    auto const footer = db.getro (footer_pos);
                    unsigned const generation = footer->a.generation;
                    db.sync (generation);
                    os1 << indent2 << "{\n";
                    if (comments) {
                        os1 << indent3 << "// generation " << generation << '\n';
                    }
                    os1 << indent3 << "\"names\": ";
                    export_names (os1, db, generation, &string_table);
                    os1 << ",\n" << indent3 << "\"debugline\": {";
                    debug_line (os1, db, generation);
                    os1 << '\n' << indent3 << "},\n";
                    os1 << indent3 << "\"fragments\": {";
                    fragments (os1, db, generation, string_table);
                    os1 << '\n' << indent3 << "},\n";
                    os1 << indent3 << "\"compilations\": {";
                    compilations (os1, db, generation, string_table);
                    os1 << '\n' << indent3 << "}\n";
                    os1 << indent2 << '}';
                });
            os << "\n}\n";
        }

    } // end namespace exchange
} // end namespace pstore
