//*                             _      __                                      _    *
//*   _____  ___ __   ___  _ __| |_   / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__| |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*           |_|                                   |___/                           *
//===- lib/exchange/export_fragment.cpp -----------------------------------===//
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
#include "pstore/exchange/export_fragment.hpp"

#include "pstore/core/hamt_map.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/diff/diff.hpp"
#include "pstore/exchange/export_emit.hpp"
#include "pstore/exchange/export_fixups.hpp"
#include "pstore/exchange/export_ostream.hpp"
#include "pstore/mcrepo/generic_section.hpp"
#include "pstore/support/base64.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace exchange {

        void export_fragments (export_ostream & os, indent const ind, pstore::database const & db,
                               unsigned const generation, export_name_mapping const & names,
                               bool comments) {
            auto const fragments =
                pstore::index::get_index<pstore::trailer::indices::fragment> (db);
            if (!fragments->empty ()) {
                auto const * fragment_sep = "\n";
                assert (generation > 0U);
                for (pstore::address const & addr :
                     pstore::diff::diff (db, *fragments, generation - 1U)) {
                    auto const & kvp = fragments->load_leaf_node (db, addr);
                    os << fragment_sep << ind << '\"' << kvp.first.to_hex_string () << R"(":)";
                    os << "{\n";
                    auto const fragment = db.getro (kvp.second);
                    auto const * section_sep = "";
                    auto const section_indent = ind.next ();
                    for (pstore::repo::section_kind const section : *fragment) {
                        os << section_sep << section_indent << '"' << section_name (section)
                           << R"(":)";
#define X(a)                                                                                       \
    case pstore::repo::section_kind::a:                                                            \
        export_section<pstore::repo::section_kind::a> (                                            \
            os, section_indent, db, names, fragment->at<pstore::repo::section_kind::a> (),         \
            comments);                                                                             \
        break;
                        switch (section) {
                            PSTORE_MCREPO_SECTION_KINDS
                        case pstore::repo::section_kind::last:
                            // llvm_unreachable...
                            assert (false);
                            break;
                        }
#undef X
                        section_sep = ",\n";
                    }
                    os << '\n' << ind << '}';
                    fragment_sep = ",\n";
                }
            }
        }

    } // end namespace exchange
} // end namespace pstore
