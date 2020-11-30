//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/brokerd/switches.cpp -----------------------------------------===//
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

#include <cassert>
#include <iostream>
#include <vector>

#include "pstore/command_line/command_line.hpp"
#include "pstore/support/utf.hpp"

using namespace pstore::command_line;

namespace {

    cl::opt<std::string>
        record_path ("record", cl::desc ("Record received messages in the named output file"));
    cl::alias record_path2 ("r", cl::desc ("Alias for --record"), cl::aliasopt (record_path));

    cl::opt<std::string> playback_path ("playback",
                                        cl::desc ("Play back messages from the named file"));
    cl::alias playback_path2 ("p", cl::desc ("Alias for --playback"), cl::aliasopt (playback_path));

    cl::opt<std::string>
        pipe_path ("pipe-path",
                   cl::desc ("Overrides the path of the FIFO from which commands will be read"));

    cl::opt<unsigned> num_read_threads ("read-threads",
                                        cl::desc ("The number of pipe reading threads"),
                                        cl::init (2U));

    cl::opt<std::uint16_t>
        http_port ("http-port",
                   cl::desc ("The port on which to listen for HTTP connections (0 to disable)"),
                   cl::init (in_port_t{8080}));

    cl::opt<unsigned>
        scavenge_time ("scavenge-time",
                       cl::desc ("The time in seconds that a message will spend in the command "
                                 "queue before being removed by the scavenger"),
                       cl::init (4U * 60U * 60U));

    std::unique_ptr<std::string> path_option (std::string const & path) {
        std::unique_ptr<std::string> result;
        if (path.length () > 0) {
            result = std::make_unique<std::string> (path);
        }
        return result;
    }

    std::unique_ptr<std::string> path_option (cl::opt<std::string> const & o) {
        return path_option (o.get ());
    }

} // end anonymous namespace


std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
    cl::parse_command_line_options (argc, argv, "pstore broker agent");

    switches result;
    result.playback_path = path_option (playback_path);
    result.record_path = path_option (record_path);
    result.pipe_path = path_option (pipe_path);
    result.num_read_threads = num_read_threads.get ();
    result.http_port = http_port.get ();
    result.scavenge_time = std::chrono::seconds{scavenge_time.get ()};
    return {std::move (result), EXIT_SUCCESS};
}
