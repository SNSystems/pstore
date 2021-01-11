//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/index_structure/switches.cpp ---------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
#include "switches.hpp"

// Standard library
#include <cstdlib>
#include <iostream>

// pstore
#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/revision_opt.hpp"
#include "pstore/command_line/str_to_revision.hpp"
#include "pstore/command_line/tchar.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/utf.hpp"

using namespace pstore::command_line;

namespace {

    opt<pstore::command_line::revision_opt, parser<std::string>>
        revision ("revision", desc ("The starting revision number (or 'HEAD')"));
    alias revision2 ("r", desc ("Alias for --revision"), aliasopt (revision));

    opt<std::string> db_path (positional, usage ("repository"), desc ("Database path"));

#define X(a) literal (#a, static_cast<int> (pstore::trailer::indices::a), #a),
    list<pstore::trailer::indices> index_names_opt (positional, optional, one_or_more,
                                                    desc ("[index-name]..."),
                                                    values ({PSTORE_INDICES}));
#undef X

    std::string usage_help () {
        std::ostringstream usage;
        usage << "pstore index structure\n\n"
                 "Dumps the internal structure of one of more pstore indexes. index-name may be "
                 "any of: ";
        pstore::gsl::czstring separator = "";
        for (literal const & lit : *index_names_opt.get_parser ()) {
            usage << separator << '\'' << lit.name << '\'';
            separator = ", ";
        }
        return usage.str ();
    }

} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
    parse_command_line_options (argc, argv, usage_help ());

    switches sw;
    sw.revision = static_cast<unsigned> (revision.get ());
    sw.db_path = db_path.get ();
    for (pstore::trailer::indices idx : index_names_opt) {
        sw.selected.set (static_cast<std::underlying_type<pstore::trailer::indices>::type> (idx));
    }
    return {sw, EXIT_SUCCESS};
}
