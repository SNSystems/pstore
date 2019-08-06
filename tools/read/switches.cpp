//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/read/switches.cpp --------------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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

#include "pstore/cmd_util/command_line.hpp"
#include "pstore/cmd_util/str_to_revision.hpp"
#include "pstore/cmd_util/revision_opt.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/utf.hpp"

namespace {

    using namespace pstore::cmd_util;

    cl::opt<pstore::cmd_util::revision_opt, false, cl::parser<std::string>>
        revision ("revision", cl::desc ("The starting revision number (or 'HEAD')"));
    cl::alias revision2 ("r", cl::desc ("Alias for --revision"), cl::aliasopt (revision));

    cl::opt<std::string> db_path (cl::Positional,
                                  cl::desc ("<Path of the pstore repository to be read>"),
                                  cl::Required);
    cl::opt<std::string> key (cl::Positional, cl::desc ("key"), cl::Required);
    cl::opt<bool>
        string_mode ("strings", cl::init (false),
                     cl::desc ("Reads from the 'strings' index rather than the 'names' index."));
    cl::alias string_mode2 ("s", cl::desc ("Alias for --strings"), cl::aliasopt (string_mode));

} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, pstore_tchar * argv[]) {
    cl::ParseCommandLineOptions (argc, argv, "pstore read utility\n");

    switches result;
    result.revision = static_cast<pstore::cmd_util::revision_opt> (revision.get ()).r;
    result.db_path = pstore::utf::from_native_string (db_path.get ());
    result.key = pstore::utf::from_native_string (key.get ());
    result.string_mode = string_mode.get ();

    return {result, EXIT_SUCCESS};
}
