//*  _ _                                _ _       _                *
//* | | |_   ___ __ ___    _____      _(_) |_ ___| |__   ___  ___  *
//* | | \ \ / / '_ ` _ \  / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* | | |\ V /| | | | | | \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |_|_| \_/ |_| |_| |_| |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                                                *
//===- tools/dump/llvm_switches.cpp ---------------------------------------===//
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
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Error.h"
#include "pstore_cmd_util/str_to_revision.hpp"
#include "pstore_cmd_util/revision_opt.hpp"
#include "pstore_support/utf.hpp"

namespace {

    using namespace llvm;

    cl::OptionCategory WhatCat ("Options controlling what is dumped");

    cl::opt<bool> Contents ("contents", cl::desc ("Emit a raw dump of the transaction contents"), cl::cat (WhatCat));
    cl::alias Contents2 ("c", cl::desc ("Alias for --contents"), cl::aliasopt (Contents));

    cl::opt<bool> Fragments ("fragments", cl::desc ("Dump the contents of the fragments index"), cl::cat (WhatCat));
	cl::opt<bool> Tickets("tickets", cl::desc("Dump the contents of the tickets index"), cl::cat(WhatCat));
    cl::opt<bool> Header ("header", cl::desc ("Dump the file header"), cl::cat (WhatCat));
    cl::alias Header2 ("h", cl::desc ("Alias for --header"), cl::aliasopt (Header));

    cl::opt<bool> Indices ("indices", cl::desc ("Dump the indices"), cl::cat (WhatCat));
    cl::alias Indices2 ("i", cl::desc ("Alias for --indices"), cl::aliasopt (Indices));

    cl::opt<bool> Log ("log", cl::desc ("List the generations"), cl::cat (WhatCat));
    cl::alias Log2 ("l", cl::desc ("Alias for --log"), cl::aliasopt (Log));

    cl::opt<bool> All ("all", cl::desc ("Show store-related output. Equivalent to: --contents --header --indices --log"), cl::cat (WhatCat));
    cl::alias All2 ("a", cl::desc ("Alias for --all"), cl::aliasopt (All));

    cl::opt<bool> SharedMemory ("shared-memory", cl::desc ("Dumps the shared-memory block"), cl::cat (WhatCat));
    cl::alias SharedMemory2 ("s", cl::desc ("Alias for --shared-memory"), cl::aliasopt (SharedMemory));


    cl::opt<pstore::cmd_util::revision_opt, false, cl::parser<std::string>> Revision (
        "revision",
        cl::desc ("The starting revision number (or 'HEAD')")
    );
    cl::alias Revision2 ("r", cl::desc ("Alias for --revision"), cl::aliasopt (Revision));


    cl::OptionCategory HowCat ("Options controlling how fields are emitted");

    cl::opt<bool> NoTimes ("no-times", cl::desc ("Times are displayed as a fixed value (for testing)"), cl::cat (HowCat));
    cl::opt<bool> Hex ("hex", cl::desc ("Emit number values in hexadecimal notation"), cl::cat (HowCat));
    cl::alias Hex2 ("x", cl::desc ("Alias for --hex"), cl::aliasopt (Hex));

    cl::opt<bool> ExpandedAddresses ("expanded-addresses", cl::desc ("Emit address values as an explicit segment/offset object"), cl::cat (HowCat));

    cl::list <std::string> Paths (cl::Positional, cl::desc ("<filename>..."));

} // anonymous namespace

std::pair <switches, int> get_switches (int argc, char * argv []) {
    llvm::cl::ParseCommandLineOptions (argc, argv, "pstore dump utility\n");

    switches result;
    result.show_contents = Contents;
    result.show_fragments = Fragments;
	result.show_tickets = Tickets;
    result.show_header = Header;
    result.show_indices = Indices;
    result.show_log = Log;
    result.show_shared = SharedMemory;
    result.show_all = All;
    result.revision = static_cast <pstore::cmd_util::revision_opt> (Revision).r;

    result.hex = Hex;
    result.no_times = NoTimes;
    result.expanded_addresses = ExpandedAddresses;

    std::transform (
        std::begin (Paths), std::end (Paths), std::back_inserter (result.paths),
        [](std::string const & path) { return pstore::utf::from_native_string (path); });

    return {result, EXIT_SUCCESS};
}

#endif // PSTORE_IS_INSIDE_LLVM
// eof: tools/dump/llvm_switches.cpp
