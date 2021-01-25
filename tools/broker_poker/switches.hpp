//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/broker_poker/switches.hpp ------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef PSTORE_BROKER_POKER_SWITCHES_HPP
#define PSTORE_BROKER_POKER_SWITCHES_HPP

#include <chrono>
#include <string>
#include <utility>

#include "pstore/command_line/tchar.hpp"
#include "pstore/config/config.hpp"
#include "pstore/support/maybe.hpp"

struct switches {
    std::string verb;
    std::string path;
    std::chrono::milliseconds retry_timeout = std::chrono::milliseconds (500);

    pstore::maybe<std::string> pipe_path;

    unsigned flood = 0;
    bool kill = false;
};

std::pair<switches, int> get_switches (int argc, pstore::command_line::tchar * argv[]);

#endif // PSTORE_BROKER_POKER_SWITCHES_HPP
