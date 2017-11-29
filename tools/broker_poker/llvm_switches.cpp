//*  _ _                                _ _       _                *
//* | | |_   ___ __ ___    _____      _(_) |_ ___| |__   ___  ___  *
//* | | \ \ / / '_ ` _ \  / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* | | |\ V /| | | | | | \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |_|_| \_/ |_| |_| |_| |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                                                *
//===- tools/broker_poker/llvm_switches.cpp -------------------------------===//
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

#include "llvm/Support/CommandLine.h"
#include "pstore_cmd_util/cl/maybe.hpp"
#include "pstore_support/utf.hpp"

namespace {

    using namespace llvm;

    cl::opt<std::string>
        PipePath ("pipe-path", cl::desc ("Overrides the FIFO path to which messages are written."),
                  cl::init (""));

    cl::opt<unsigned> Flood ("flood", cl::desc ("Flood the broker with a number of ECHO messages."),
                             cl::init (0));
    cl::alias Flood2 ("m", cl::desc ("Alias for --flood"), cl::aliasopt (Flood));

    cl::opt<unsigned>
        RetryTimeout ("retry-timeout",
                      cl::desc ("The timeout for connection retries to the broker (ms)."),
                      cl::init (switches{}.retry_timeout.count ()));
    cl::opt<unsigned>
        MaxRetries ("max-retries",
                    cl::desc ("The maximum number of retries that will be attempted."),
                    cl::init (switches{}.max_retries));

    cl::opt<bool> Kill ("kill",
                        cl::desc ("Ask the broker to quit after commands have been processed."));
    cl::alias Kill2 ("k", cl::desc ("Alias for --kill"), cl::aliasopt (Kill));

    cl::opt<std::string> Verb (cl::Positional, cl::Optional, cl::desc ("<verb>"));
    cl::opt<std::string> Path (cl::Positional, cl::Optional, cl::desc ("<path>"));

    auto pathOption (std::string const & path) -> pstore::cmd_util::cl::maybe<std::string> {
        if (path.length () > 0) {
            return pstore::cmd_util::cl::just (path);
        }
        return pstore::cmd_util::cl::nothing<std::string> ();
    }

} // anonymous namespace

std::pair<switches, int> get_switches (int argc, char * argv[]) {
    llvm::cl::ParseCommandLineOptions (argc, argv, "pstore broker poker\n");

    switches Result;
    Result.verb = pstore::utf::from_native_string (Verb);
    Result.path = pstore::utf::from_native_string (Path);
    Result.retry_timeout = std::chrono::milliseconds (RetryTimeout);
    Result.max_retries = MaxRetries;
    Result.flood = Flood;
    Result.kill = Kill;
    Result.pipe_path = pathOption (PipePath);
    return {Result, EXIT_SUCCESS};
}

#endif // PSTORE_IS_INSIDE_LLVM
// eof: tools/broker_poker/llvm_switches.cpp
