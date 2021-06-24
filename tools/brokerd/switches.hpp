//===- tools/brokerd/switches.hpp -------------------------*- mode: C++ -*-===//
//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef SWITCHES_HPP
#define SWITCHES_HPP

#include <chrono>
#include <memory>
#include <string>
#include <tuple>

#include "pstore/command_line/tchar.hpp"
#include "pstore/os/descriptor.hpp" // for in_port_t
#include "pstore/support/maybe.hpp"

struct switches {
    pstore::maybe<std::string> playback_path;
    pstore::maybe<std::string> record_path;
    pstore::maybe<std::string> pipe_path;
    unsigned num_read_threads = 2U;
    bool announce_http_port = false;
    pstore::maybe<in_port_t> http_port;
    std::chrono::seconds scavenge_time;
};

std::pair<switches, int> get_switches (int argc, pstore::command_line::tchar * argv[]);

#endif // SWITCHES_HPP
