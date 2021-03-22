//===- tools/dump/switches.hpp ----------------------------*- mode: C++ -*-===//
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
/// \file switches.hpp
/// \brief Defines a structure which represents the dump tool's command-line switches.
#ifndef PSTORE_DUMP_SWITCHES_HPP
#define PSTORE_DUMP_SWITCHES_HPP

#include <list>
#include <string>
#include <utility>

#include "pstore/command_line/tchar.hpp"
#include "pstore/config/config.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/index_types.hpp"

struct switches {
    bool show_all = false;

    /// A list of the individual fragment digests from the command-line.
    std::list<pstore::index::digest> fragments;
    /// True if --all-fragments was specified on the command-line.
    bool show_all_fragments = false;
    /// A list containing compilations digests from the command-line.
    std::list<pstore::index::digest> compilations;
    /// True is --all-compilations was specified on the command-line.
    bool show_all_compilations = false;
    /// A list of the individual debug line header digests from the command-line.
    std::list<pstore::index::digest> debug_line_headers;
    /// True if --all-debug-line-headers was specified on the command-line.
    bool show_all_debug_line_headers = false;
    /// The target-triple to use for disassembly if one is not known.
    std::string triple;

    bool show_header = false;
    bool show_indices = false;
    bool show_log = false;
    bool show_shared = false;

    /// True if --names was specified.
    bool show_names = false;
    /// True if --paths was specified.
    bool show_paths = false;

    unsigned revision = pstore::head_revision;

    bool hex = false;
    bool expanded_addresses = false;
    bool no_times = false;

    std::list<std::string> paths;
};

std::pair<switches, int> get_switches (int argc, pstore::command_line::tchar * argv[]);

#endif // PSTORE_DUMP_SWITCHES_HPP
