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
#include "pstore/exchange/export_section.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/mcrepo/generic_section.hpp"
#include "pstore/mcrepo/section.hpp"
#include "pstore/support/base64.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace exchange {

        void fragments (crude_ostream & os, pstore::database const & db, unsigned const generation,
                        export_name_mapping const & names) {
            auto fragments = pstore::index::get_index<pstore::trailer::indices::fragment> (db);
            if (!fragments->empty ()) {
                auto const * fragment_sep = "\n";
                assert (generation > 0U);
                for (pstore::address const & addr :
                     pstore::diff::diff (db, *fragments, generation - 1U)) {
                    auto const & kvp = fragments->load_leaf_node (db, addr);
                    os << fragment_sep << indent4 << '\"' << kvp.first.to_hex_string ()
                       << "\": {\n";

                    auto fragment = db.getro (kvp.second);
                    auto const * section_sep = "";
                    for (pstore::repo::section_kind section : *fragment) {
                        os << section_sep << indent5 << '"' << section_name (section) << "\": {\n";
#define X(a)                                                                                       \
    case pstore::repo::section_kind::a:                                                            \
        emit_section<pstore::repo::section_kind::a> (                                              \
            os, db, names, fragment->at<pstore::repo::section_kind::a> ());                        \
        break;
                        switch (section) {
                            PSTORE_MCREPO_SECTION_KINDS
                        case pstore::repo::section_kind::last:
                            // llvm_unreachable...
                            assert (false);
                            break;
                        }
#undef X
                        os << indent5 << '}';
                        section_sep = ",\n";
                    }
                    os << '\n' << indent4 << '}';
                    fragment_sep = ",\n";
                }
            }
        }

    } // end namespace exchange
} // end namespace pstore
