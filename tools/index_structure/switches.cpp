//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/index_structure/switches.cpp ---------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
