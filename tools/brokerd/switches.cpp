//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/brokerd/switches.cpp -----------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "switches.hpp"

#include <cassert>
#include <iostream>
#include <vector>

#include "pstore/command_line/command_line.hpp"
#include "pstore/support/utf.hpp"

using namespace pstore::command_line;

namespace {

    opt<std::string> record_path{"record",
                                 desc{"Record received messages in the named output file"}};
    alias record_path2{"r", desc{"Alias for --record"}, aliasopt{record_path}};

    opt<std::string> playback_path{"playback", desc{"Play back messages from the named file"}};
    alias playback_path2{"p", desc{"Alias for --playback"}, aliasopt{playback_path}};

    opt<std::string> pipe_path{
        "pipe-path", desc{"Overrides the path of the FIFO from which commands will be read"}};

    opt<unsigned> num_read_threads{"read-threads", desc{"The number of pipe reading threads"},
                                   init (2U)};

    opt<std::uint16_t> http_port{"http-port",
                                 desc{"The port on which to listen for HTTP connections"},
                                 init (in_port_t{8080})};
    opt<bool> disable_http{"disable-http", desc{"Disable the HTTP server"}, init (false)};

    opt<bool> announce_http_port{"announce-http-port",
                                 desc{"Display a message when the HTTP server is available"},
                                 init (false)};

    opt<unsigned> scavenge_time{"scavenge-time",
                                desc{"The time in seconds that a message will spend in the command "
                                     "queue before being removed by the scavenger"},
                                init (4U * 60U * 60U)};

    pstore::maybe<std::string> path_option (opt<std::string> const & path) {
        if (path.get_num_occurrences () == 0U) {
            return pstore::nothing<std::string> ();
        }
        return pstore::just (path.get ());
    }

} // end anonymous namespace


std::pair<switches, int> get_switches (int const argc, tchar * argv[]) {
    parse_command_line_options (argc, argv, "pstore broker agent");

    switches result;
    result.playback_path = path_option (playback_path);
    result.record_path = path_option (record_path);
    result.pipe_path = path_option (pipe_path);
    result.num_read_threads = num_read_threads.get ();
    result.announce_http_port = announce_http_port.get ();
    result.http_port =
        disable_http ? pstore::nothing<in_port_t> () : pstore::just (http_port.get ());
    result.scavenge_time = std::chrono::seconds{scavenge_time.get ()};
    return {std::move (result), EXIT_SUCCESS};
}
