//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/vacuum/switches.cpp ------------------------------------------===//
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

#if !PSTORE_IS_INSIDE_LLVM

#include <cstdlib>
#include <iostream>
#include <vector>
// 3rd party
#include "optionparser.h"

#include "pstore_support/utf.hpp"

namespace {
#if defined(_WIN32) && defined(_UNICODE)
    auto & out_stream = std::wcout;
#else
    auto & out_stream = std::cout;
#endif


    enum option_index {
        unknown_opt,
        help_opt,
        // daemon_opt,
        // verbose_opt,
    };

    option::Descriptor const usage[] = {
        {unknown_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT (""), option::Arg::None,
         NATIVE_TEXT ("Usage: vacuumd [options] data-file \n\nOptions:")},
        {help_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT ("help"), option::Arg::None,
         NATIVE_TEXT ("  --help \tPrint usage and exit.")},

        {unknown_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT (""), option::Arg::None,
         NATIVE_TEXT ("\nExamples:\n"
                      "  vacuumd foo.db\n"
                      "Remove old revisions from the given data store.\n")},

        {0, 0, 0, 0, 0, 0},
    };
} // end anonymous namespace

std::pair<vacuum::user_options, int> get_switches (int argc, TCHAR * argv[]) {
    int exit_code = EXIT_SUCCESS;
    vacuum::user_options opt;

    // Skip program name argv[0] if present
    argc -= (argc > 0);
    argv += (argc > 0);

    option::Stats stats (usage, argc, argv);
    std::vector<option::Option> options (stats.options_max);
    std::vector<option::Option> buffer (stats.buffer_max);
    option::Parser parse (usage, argc, argv, options.data (), buffer.data ());
    if (parse.error ()) {
        exit_code = EXIT_FAILURE;
    }
    if (exit_code == EXIT_SUCCESS) {
        if (options[help_opt] || parse.nonOptionsCount () != 1) {
            option::printUsage (out_stream, usage);
            exit_code = EXIT_FAILURE;
        }
    }
    if (exit_code == EXIT_SUCCESS) {
        if (option::Option const * unknown = options[unknown_opt]) {
            for (; unknown != nullptr; unknown = unknown->next ()) {
                out_stream << NATIVE_TEXT ("Unknown option: ") << unknown->name
                           << NATIVE_TEXT ("\n");
            }
            exit_code = EXIT_FAILURE;
        }
    }
    if (exit_code == EXIT_SUCCESS) {
        opt.src_path = pstore::utf::from_native_string (parse.nonOption (0));
    }

    return {opt, exit_code};
}

#endif // PSTORE_IS_INSIDE_LLVM
// eof:tools/vacuum/switches.cpp

// eof: tools/vacuum/switches.cpp
