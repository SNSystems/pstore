//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/diff/switches.cpp --------------------------------------------===//
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

#include "switches.hpp"

#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/str_to_revision.hpp"
#include "pstore/command_line/revision_opt.hpp"

using namespace pstore::command_line;

namespace {

    opt<std::string> db_path (positional, usage ("repository"),
                              desc ("Path of the pstore repository to be read."), required);

    opt<pstore::command_line::revision_opt, parser<std::string>>
        first_revision (positional, usage ("[1st-revision]"),
                        desc ("The first revision number (or 'HEAD')"), optional);

    opt<pstore::command_line::revision_opt, parser<std::string>>
        second_revision (positional, usage ("[2nd-revision]"),
                         desc ("The second revision number (or 'HEAD')"), optional);

    option_category how_cat ("Options controlling how fields are emitted");

    opt<bool> hex ("hex", desc ("Emit number values in hexadecimal notation"), cat (how_cat));
    alias hex2 ("x", desc ("Alias for --hex"), aliasopt (hex));

} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
    using namespace pstore;
    parse_command_line_options (argc, argv, "pstore diff utility\n");

    switches result;
    result.db_path = db_path.get ();
    result.first_revision = static_cast<unsigned> (first_revision.get ());
    result.second_revision = second_revision.get_num_occurrences () > 0
                                 ? just (static_cast<unsigned> (second_revision.get ()))
                                 : nothing<revision_number> ();
    result.hex = hex.get ();
    return {result, EXIT_SUCCESS};
}
