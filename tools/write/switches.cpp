//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/write/switches.cpp -------------------------------------------===//
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

#include <cctype>
#include <iostream>

// 3rd party
#include "optionparser.h"

#include "pstore_support/error.hpp"
#include "pstore_support/gsl.hpp"
#include "pstore_support/utf.hpp"
#include "pstore/database.hpp"

#include "error.hpp"
#include "to_value_pair.hpp"

namespace {

#if defined(_WIN32) && defined(_UNICODE)
    auto & error_stream = std::wcerr;
    auto & out_stream = std::wcout;
#else
    auto & error_stream = std::cerr;
    auto & out_stream = std::cout;
#endif

    option::ArgStatus add (option::Option const & option, bool msg,
                           ::pstore::gsl::czstring value_kind) {
        std::pair<std::string, std::string> resl =
            to_value_pair (pstore::utf::from_native_string (option.arg).c_str ());
        if (resl.first.length () == 0 && resl.second.length () == 0) {
            if (msg) {
                error_stream << NATIVE_TEXT ("Option '") << option.name
                             << NATIVE_TEXT ("' requires a command-separated key and " << value_kind
                                                                                       << '\n');
            }
            return option::ARG_ILLEGAL;
        }
        return option::ARG_OK;
    }
    option::ArgStatus add_value (option::Option const & option, bool msg) {
        return add (option, msg, "value");
    }
    option::ArgStatus add_file (option::Option const & option, bool msg) {
        return add (option, msg, "file path");
    }

    option::ArgStatus add_string (option::Option const & option, bool msg) {
        if (option.arg == nullptr || option.arg [0] == '\0') {
            if (msg) {
                error_stream << NATIVE_TEXT ("Option '") << option.name
                             << NATIVE_TEXT ("' requires a key string\n'");
            }
            return option::ARG_ILLEGAL;
        }
        return option::ARG_OK;
    }

     pstore::database::vacuum_mode to_vacuum_mode (std::string const & opt) {
        if (opt == "disabled") {
            return pstore::database::vacuum_mode::disabled;
        } else if (opt == "immediate") {
            return pstore::database::vacuum_mode::immediate;
        } else if (opt == "background") {
            return pstore::database::vacuum_mode::background;
        }

        pstore::raise_error_code (std::make_error_code (write_error_code::unrecognized_compaction_mode));
    }

    option::ArgStatus compact_mode (option::Option const & option, bool msg) {
        try {
            to_vacuum_mode (pstore::utf::from_native_string (option.arg).c_str ());
        } catch (std::exception const & ex) {
            if (msg) {
                std::cerr << "Option '" << option.name << "' " << ex.what () << '\n';
            }
            return option::ARG_ILLEGAL;
        }
        return option::ARG_OK;
    }


    enum option_index {
        unknown_opt,
        help_opt,
        add_opt,
        add_string_opt,
        add_file_opt,
        compact_opt,
    };

    option::Descriptor const usage[] = {
        {unknown_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT (""), option::Arg::None,
         NATIVE_TEXT ("Usage: write [options] data-file [files-to-store]\n\nOptions:")},
        {help_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT ("help"), option::Arg::None,
         NATIVE_TEXT ("  --help        \tPrint usage and exit.")},
        {add_opt, 0, NATIVE_TEXT ("a"), NATIVE_TEXT ("add"), add_value,
         NATIVE_TEXT ("  -a,--add      \t"
                      "Add key with corresponding string value. Specified as 'key,value'."
                      " May be repeated to add several keys.")},
        {add_string_opt, 0, NATIVE_TEXT ("s"), NATIVE_TEXT ("add-string"), add_string,
         NATIVE_TEXT ("  -s,--add-string      \t"
                      "Add key to string set. Specified as 'key'."
                      " May be repeated to add several keys.")},
        {add_file_opt, 0, NATIVE_TEXT ("f"), NATIVE_TEXT ("add-file"), add_file,
         NATIVE_TEXT (
             "  -f,--add-file \t"
             "Add key with corresponding value from the given path (specified as 'key,path')."
             " May be repeated to add several keys.")},
        {compact_opt, 0, NATIVE_TEXT ("n"), NATIVE_TEXT ("compact"), compact_mode,
         NATIVE_TEXT ("  -c,--compact  \t"
                      "Set the compaction mode. Argument must one of: 'off', "
                      "'now', 'delayed'.")},
        {unknown_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT (""), option::Arg::None,
         NATIVE_TEXT ("\nExamples:\n"
                      "  write \t Writes some stuff somewhere.\n")},

        {0, 0, 0, 0, 0, 0},
    };

} // (anonymous namespace)


std::pair <switches, int> get_switches (int argc, TCHAR * argv []) {
    switches sw;

    argc -= (argc > 0);
    argv += (argc > 0);

    option::Stats stats (usage, argc, argv);
    std::vector<option::Option> options (stats.options_max);
    std::vector<option::Option> buffer (stats.buffer_max);
    option::Parser parse (usage, argc, argv, options.data (), buffer.data ());
    if (parse.error ()) {
        return {sw, EXIT_FAILURE};
    }

    if (options[help_opt] || parse.nonOptionsCount () < 1) {
        option::printUsage (out_stream, usage);
        return {sw, EXIT_FAILURE};
    }

    if (option::Option const * const opt = options[compact_opt]) {
        sw.vmode = to_vacuum_mode (pstore::utf::from_native_string (opt->arg));
    }

    for (option::Option const * opt = options[add_opt]; opt != nullptr; opt = opt->next ()) {
        sw.add.push_back (to_value_pair (pstore::utf::from_native_string (opt->arg)));
    }
    for (option::Option const * opt = options[add_string_opt]; opt != nullptr; opt = opt->next ()) {
        sw.strings.push_back (pstore::utf::from_native_string (opt->arg));
    }
    for (option::Option const * opt = options[add_file_opt]; opt != nullptr; opt = opt->next ()) {
        sw.files.push_back (to_value_pair (pstore::utf::from_native_string (opt->arg)));
    }

    sw.db_path = pstore::utf::from_native_string (parse.nonOption (0));

    for (int count = 1, non_options_count = parse.nonOptionsCount (); count < non_options_count; ++count) {
        std::string const path = pstore::utf::from_native_string (parse.nonOption (count));
        sw.files.emplace_back (path, path);
    }

    return {sw, EXIT_SUCCESS};
}
#endif // PSTORE_IS_INSIDE_LLVM
// eof: tools/write/switches.cpp
