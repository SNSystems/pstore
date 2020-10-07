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
#include "pstore/exchange/export_compilation.hpp"
#include "pstore/exchange/export_emit.hpp"
#include "pstore/exchange/export_fragment.hpp"
#include "pstore/exchange/export_names.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/mcrepo/generic_section.hpp"
#include "pstore/mcrepo/section.hpp"
#include "pstore/support/base64.hpp"
#include "pstore/support/gsl.hpp"

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

    void export_debug_line (pstore::exchange::export_ostream & os,
                            pstore::exchange::indent const ind, pstore::database const & db,
                            unsigned const generation) {
        auto const debug_line_headers =
            pstore::index::get_index<pstore::trailer::indices::debug_line_header> (db);
        if (!debug_line_headers->empty ()) {
            auto const * sep = "\n";
            assert (generation > 0);
            for (pstore::address const & addr :
                 pstore::diff::diff (db, *debug_line_headers, generation - 1U)) {
                auto const & kvp = debug_line_headers->load_leaf_node (db, addr);
                os << sep << ind << '"' << kvp.first.to_hex_string () << R"(":")";
                std::shared_ptr<std::uint8_t const> const data = db.getro (kvp.second);
                auto const * const ptr = data.get ();
                pstore::to_base64 (ptr, ptr + kvp.second.size,
                                   pstore::exchange::ostream_inserter{os});
                os << '"';

                sep = ",\n";
            }
        }
    }

} // end anonymous namespace

namespace pstore {
    namespace exchange {

        void export_database (database & db, export_ostream & os, bool comments) {
            export_name_mapping string_table;
            auto const ind = indent{}.next ();
            os << "{\n";
            os << ind << R"("version":1,)" << '\n';
            os << ind << R"("id":")" << db.get_header ().id ().str () << "\",\n";
            os << ind << R"("transactions":)";

            auto const f = footers (db);
            assert (std::distance (std::begin (f), std::end (f)) >= 1);
            emit_array (os, ind, std::next (std::begin (f)), std::end (f),
                        [&] (export_ostream & os1, indent const ind1,
                             pstore::typed_address<pstore::trailer> const footer_pos) {
                            auto const footer = db.getro (footer_pos);
                            unsigned const generation = footer->a.generation;
                            db.sync (generation);
                            os1 << ind1 << "{\n";
                            auto const object_indent = ind1.next ();
                            if (comments) {
                                os1 << object_indent << "// generation " << generation << '\n';
                            }
                            os1 << object_indent << R"("names":)";
                            export_names (os1, object_indent, db, generation, &string_table);
                            os1 << ",\n" << object_indent << R"("debugline":{)";
                            export_debug_line (os1, object_indent.next (), db, generation);
                            os1 << '\n' << object_indent << "},\n";
                            os1 << object_indent << R"("fragments":{)";
                            export_fragments (os1, object_indent.next (), db, generation,
                                              string_table, comments);
                            os1 << '\n' << object_indent << "},\n";
                            os1 << object_indent << R"("compilations":{)";
                            export_compilation_index (os1, object_indent.next (), db, generation,
                                                      string_table, comments);
                            os1 << '\n' << object_indent << "}\n";
                            os1 << ind1 << '}';
                        });
            os << "\n}\n";
        }

    } // end namespace exchange
} // end namespace pstore
