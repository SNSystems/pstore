//*  _ _                                _ _       _                *
//* | | |_   ___ __ ___    _____      _(_) |_ ___| |__   ___  ___  *
//* | | \ \ / / '_ ` _ \  / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* | | |\ V /| | | | | | \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |_|_| \_/ |_| |_| |_| |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                                                *
//===- tools/index_structure/llvm_switches.cpp ----------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
#include <cstdlib>
#include <iostream>
#include "llvm/Support/CommandLine.h"

// pstore
#include "pstore_cmd_util/str_to_revision.hpp"
#include "pstore_cmd_util/revision_opt.hpp"
#include "pstore_support/utf.hpp"

namespace {

#if defined(_WIN32) && defined(_UNICODE)
    auto & error_stream = std::wcerr;
#else
    auto & error_stream = std::cerr;
#endif

    using namespace llvm;

    cl::opt<pstore::cmd_util::revision_opt, false, cl::parser<std::string>> Revision (
        "revision",
        cl::desc ("The starting revision number (or 'HEAD')")
    );
    cl::alias Revision2 ("r", cl::desc ("Alias for --revision"), cl::aliasopt (Revision));

    cl::opt<std::string> DbPath (cl::Positional, cl::desc("database-path"));
    cl::list <std::string> IndexNames (cl::Positional, cl::Optional, cl::OneOrMore, cl::desc ("<index-name>..."));


    std::string usage_help () {
        std::ostringstream usage;
        usage << "pstore index structure\n\n";

        usage << "Dumps the internal structure of one of more pstore indexes. index-name may be any of: ";
        char const * separator = "";
        for (auto const & in: index_names) {
            usage << separator << '\'' << in << '\'';
            separator = ", ";
        }
        usage << " ('*' may be used as a shortcut for all names).\n";
        return usage.str ();
    }

} // (anonymous namespace)

std::pair<switches, int> get_switches (int argc, pstore_tchar * argv []) {
    llvm::cl::ParseCommandLineOptions (argc, argv, usage_help ());

    switches sw;
    sw.revision = static_cast <pstore::cmd_util::revision_opt> (Revision).r;
    sw.db_path = DbPath;

    for (auto const & Name: IndexNames) {
        if (!set_from_name (&sw.selected, Name)) {
            error_stream << NATIVE_TEXT ("Unknown index ") << pstore::utf::to_native_string (Name) << NATIVE_TEXT ("\n");
            return {sw, EXIT_FAILURE};
        }
    }
    return {sw, EXIT_SUCCESS};
}
#endif //PSTORE_IS_INSIDE_LLVM

// eof: tools/index_structure/llvm_switches.cpp
