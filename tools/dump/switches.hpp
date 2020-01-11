//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/dump/switches.hpp --------------------------------------------===//
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
#ifndef PSTORE_DUMP_SWITCHES_HPP
#define PSTORE_DUMP_SWITCHES_HPP

#include <list>
#include <string>
#include <utility>

#include "pstore/cmd_util/tchar.hpp"
#include "pstore/config/config.hpp"
#include "pstore/core/database.hpp"
#include "pstore/core/index_types.hpp"

struct switches {
    bool show_all = false;
    bool show_contents = false;

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

    unsigned revision = pstore::head_revision;

    bool hex = false;
    bool expanded_addresses = false;
    bool no_times = false;

    std::list<std::string> paths;
};

std::pair<switches, int> get_switches (int argc, pstore::cmd_util::tchar * argv[]);

#endif // PSTORE_DUMP_SWITCHES_HPP
