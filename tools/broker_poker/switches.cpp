//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/broker_poker/switches.cpp ------------------------------------===//
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

#include <cstdlib>

#include "pstore/cmd_util/command_line.hpp"
#include "pstore/support/maybe.hpp"

using namespace pstore::cmd_util;

namespace {

    cl::opt<std::string>
        pipe_path ("pipe-path", cl::desc ("Overrides the FIFO path to which messages are written."),
                   cl::init (""));

    cl::opt<unsigned> flood ("flood", cl::desc ("Flood the broker with a number of ECHO messages."),
                             cl::init (0U));
    cl::alias flood2 ("m", cl::desc ("Alias for --flood"), cl::aliasopt (flood));

    cl::opt<std::chrono::milliseconds::rep>
        retry_timeout ("retry-timeout",
                       cl::desc ("The timeout for connection retries to the broker (ms)."),
                       cl::init (switches{}.retry_timeout.count ()));

    cl::opt<bool> kill ("kill",
                        cl::desc ("Ask the broker to quit after commands have been processed."));
    cl::alias kill2 ("k", cl::desc ("Alias for --kill"), cl::aliasopt (kill));

    cl::opt<std::string> verb (cl::Positional, cl::Optional, cl::desc ("<verb>"));
    cl::opt<std::string> path (cl::Positional, cl::Optional, cl::desc ("<path>"));

    pstore::maybe<std::string> path_option (std::string const & p) {
        if (p.length () > 0) {
            return pstore::just (p);
        }
        return pstore::nothing<std::string> ();
    }

} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
    cl::ParseCommandLineOptions (argc, argv, "pstore broker poker\n");

    switches result;
    result.verb = verb.get ();
    result.path = path.get ();
    result.retry_timeout = std::chrono::milliseconds (retry_timeout.get ());
    result.flood = flood.get ();
    result.kill = kill.get ();
    result.pipe_path = path_option (pipe_path.get ());
    return {result, EXIT_SUCCESS};
}
