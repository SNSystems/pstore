//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/diff/switches.cpp --------------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

#if PSTORE_IS_INSIDE_LLVM
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"
#else
#include "pstore/cmd_util/cl/command_line.hpp"
#endif

#include "pstore/cmd_util/str_to_revision.hpp"
#include "pstore/cmd_util/revision_opt.hpp"
#include "pstore/support/utf.hpp"

#if PSTORE_IS_INSIDE_LLVM
using namespace llvm;
#else
using namespace pstore::cmd_util;
#endif

namespace {
    cl::opt<std::string> DbPath (cl::Positional,
                                 cl::desc ("Path of the pstore repository to be read."),
                                 cl::Required);

    cl::opt<pstore::cmd_util::revision_opt, false, cl::parser<std::string>>
        FirstRevision (cl::Positional, cl::desc ("The first revision number (or 'HEAD')"),
                       cl::Optional);

    cl::opt<pstore::cmd_util::revision_opt, false, cl::parser<std::string>>
        SecondRevision (cl::Positional, cl::desc ("The second revision number (or 'HEAD')"),
                        cl::Optional);

    cl::OptionCategory HowCat ("Options controlling how fields are emitted");

    cl::opt<bool> Hex ("hex", cl::desc ("Emit number values in hexadecimal notation"),
                       cl::cat (HowCat));
    cl::alias Hex2 ("x", cl::desc ("Alias for --hex"), cl::aliasopt (Hex));

} // anonymous namespace

std::pair<switches, int> get_switches (int argc, pstore_tchar * argv[]) {
    using namespace pstore;
    cl::ParseCommandLineOptions (argc, argv, "pstore diff utility\n");

    switches result;
    result.db_path = utf::from_native_string (DbPath);
    result.first_revision = static_cast<cmd_util::revision_opt> (FirstRevision).r;
    result.second_revision = SecondRevision.getNumOccurrences () > 0
                                 ? just (static_cast<cmd_util::revision_opt> (SecondRevision).r)
                                 : nothing<diff::revision_number> ();

    result.hex = Hex;

    return {result, EXIT_SUCCESS};
}

// eof: tools/diff/switches.cpp
