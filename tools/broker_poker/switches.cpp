//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/broker_poker/switches.cpp ------------------------------------===//
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
#include <iostream>
#include <vector>
#include "optionparser.h"
#include "pstore_support/utf.hpp"
namespace {
#if defined(_WIN32) && defined(_UNICODE)
    auto & error_stream = std::wcerr;
    unsigned long str_to_ul (TCHAR const * str, TCHAR ** str_end) {
        return std::wcstoul (str, str_end, 10);
    }
#else
    auto & error_stream = std::cerr;
    unsigned long str_to_ul (char const * str, char ** str_end) {
        return std::strtoul (str, str_end, 10);
    }
#endif
} // (anonymous namespace)


namespace {
    option::ArgStatus numeric (option::Option const & option, bool msg) {
        constexpr auto max = std::numeric_limits <unsigned>::max ();
        assert (option.name != nullptr);
        TCHAR * endptr = nullptr;
        unsigned long value = 0UL;
        if (option.arg != nullptr) {
            value = str_to_ul (option.arg, &endptr);
        }
        if (endptr != option.arg && *endptr == '\0' && value <= max) {
            return option::ARG_OK;
        }

        if (msg) {
            if (value > std::numeric_limits <unsigned>::max ()) {
                error_stream << NATIVE_TEXT ("Option '") << option.name
                             << NATIVE_TEXT ("': value too large\n");
            } else {
                error_stream << NATIVE_TEXT ("Option '") << option.name
                             << NATIVE_TEXT ("' requires a numeric argument ";
                if (option.arg != nullptr) {
                    error_stream << "(got \"") << option.arg;
                } else {
                    error_stream << " (but was missing)";
                }
                error_stream << NATIVE_TEXT ("\")\n");
            }
        }
        return option::ARG_ILLEGAL;
    }

    enum option_index {
        flood_opt,
        help_opt,
        kill_opt,
        retry_opt,
        max_retries_opt,
        unknown_opt,
    };

#define USAGE "Usage: broker_poker [options] [verb [path]]"

    option::Descriptor const usage[] = {
        {unknown_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT (""), option::Arg::None,
         NATIVE_TEXT (USAGE "\n\nOptions:")},

        {help_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT ("help"), option::Arg::None,
         NATIVE_TEXT ("  --help        \tPrint usage and exit.")},
        {flood_opt, 0, NATIVE_TEXT ("f"), NATIVE_TEXT ("flood"), numeric,
         NATIVE_TEXT ("  -f,--flood num     \t"
                      "Flood the broker with 'num' ECHO messages.")},
        {kill_opt, 0, NATIVE_TEXT ("k"), NATIVE_TEXT ("kill"), option::Arg::None,
         NATIVE_TEXT ("  -k,--kill      \t"
                      "Ask the broker to quit after commands have been processed.")},

        {retry_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT ("retry-timeout"), option::Arg::None,
         NATIVE_TEXT ("  --retry-timeout      \t"
                      "Retry timeout.")},
        {max_retries_opt, 0, NATIVE_TEXT(""), NATIVE_TEXT ("max-retries"), numeric,
         NATIVE_TEXT ("  --max-retries      \t"
                      "Maximum number of retries that will be attempted.")},

        {0, 0, 0, 0, 0, 0},
    };

} // (anonymous namespace)

std::pair<switches, int> get_switches (int argc, TCHAR * argv []) {
    switches result;

    // Skip program name argv[0] if present
    argc -= (argc > 0);
    argv += (argc > 0);

    option::Stats stats (usage, argc, argv);
    std::vector<option::Option> options (stats.options_max);
    std::vector<option::Option> buffer (stats.buffer_max);
    option::Parser parse (usage, argc, argv, options.data (), buffer.data ());
    if (parse.error ()) {
        return {result, EXIT_FAILURE};
    }

    if (options[help_opt]) {
        option::printUsage (error_stream, usage);
        return {result, EXIT_FAILURE};
    }
    auto const num_non_option_arguments = parse.nonOptionsCount ();
    if (num_non_option_arguments > 2) {
        std::cerr << USAGE << '\n';
        return {result, EXIT_FAILURE};
    }

    if (auto & flood = options[flood_opt]) {
        result.flood = static_cast <unsigned> (str_to_ul (flood.arg, nullptr));
    } 
    if (auto & retry = options[retry_opt]) {
        result.retry_timeout = std::chrono::milliseconds (str_to_ul (retry.arg, nullptr));
    } 
    if (auto & max_wait = options[max_retries_opt]) {
        result.max_retries = static_cast <unsigned> (str_to_ul (max_wait.arg, nullptr));
    } 

    if (num_non_option_arguments > 0) {
        auto const non_opts = parse.nonOptions ();
        result.verb = pstore::utf::from_native_string (non_opts[0]);
        if (num_non_option_arguments == 2) {
            result.path = pstore::utf::from_native_string (non_opts[1]);
        }
    }

    result.kill = options[kill_opt];
    return {result, EXIT_SUCCESS};
}

#endif // !PSTORE_IS_INSIDE_LLVM
// eof: tools/broker_poker/switches.cpp
