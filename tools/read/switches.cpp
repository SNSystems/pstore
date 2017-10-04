//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/read/switches.cpp --------------------------------------------===//
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
// pstore
#include "pstore_cmd_util/str_to_revision.h"
#include "pstore_support/error.hpp"
#include "pstore_support/gsl.hpp"
#include "pstore_support/utf.hpp"

#include "switches.hpp"

namespace {
#if defined(_WIN32) && defined(_UNICODE)
    auto & error_stream = std::wcerr;
    auto & out_stream = std::wcout;
#else
    auto & error_stream = std::cerr;
    auto & out_stream = std::cout;
#endif

    enum option_index {
        unknown_opt,
        help_opt,
        revision_opt,
        string_opt,
    };

    option::ArgStatus revision (option::Option const & option, bool msg) {
        auto r = pstore::str_to_revision (pstore::utf::from_native_string (option.arg));
        if (r.second) {
            return option::ARG_OK;
        }
        if (msg) {
            error_stream << "Option '" << option
                         << "' requires an argument which may be a revision number or 'HEAD'\n";
        }
        return option::ARG_ILLEGAL;
    }


#define USAGE "Usage: read [options] data-file key"

    option::Descriptor const usage[] = {
        {unknown_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT (""), option::Arg::None,
         NATIVE_TEXT (USAGE "\n\nOptions:")},
        {help_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT ("help"), option::Arg::None,
         NATIVE_TEXT ("  --help \tPrint usage and exit.")},

        {revision_opt, 0, NATIVE_TEXT ("r"), NATIVE_TEXT ("revision"), revision,
         NATIVE_TEXT ("  --revision, -r  \tThe pstore revision from which to read (or 'HEAD').")},
        {string_opt, 0, NATIVE_TEXT ("s"), NATIVE_TEXT ("strings"), option::Arg::None,
         NATIVE_TEXT (
             "  --strings, -s  \tReads from the 'strings' index rather than the 'names' index.")},

        {unknown_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT (""), option::Arg::None,
         NATIVE_TEXT ("\nExamples:\n"
                      "  read foo.db key1 \t"
                      "Reads the value associated with 'key1' at the HEAD revision of pstore "
                      "foo.db and writes it to stdout\n"
                      "  read -r 1 foo.db key1 \t"
                      "Reads the value associated with 'key1' at revision 1 of pstore foo.db and "
                      "writes it to stdout\n"
                      "  read --string foo.db str1 \t"
                      "Reads the value associated with 'str1' in the strings index at the HEAD "
                      "revision")},

        {0, 0, 0, 0, 0, 0},
    };
} // namespace

std::pair<switches, int> get_switches (int argc, TCHAR * argv[]) {
    switches sw;

    // Skip program name argv[0] if present
    argc -= (argc > 0);
    argv += (argc > 0);

    option::Stats stats (usage, argc, argv);
    std::vector<option::Option> options (stats.options_max);
    std::vector<option::Option> buffer (stats.buffer_max);
    option::Parser parse (usage, argc, argv, options.data (), buffer.data ());
    if (parse.error ()) {
        return {sw, EXIT_FAILURE};
    }
    if (options[help_opt]) {
        option::printUsage (out_stream, usage);
        return {sw, EXIT_FAILURE};
    }

    if (option::Option const * unknown = options[unknown_opt]) {
        for (; unknown != nullptr; unknown = unknown->next ()) {
            out_stream << NATIVE_TEXT ("Unknown option: ") << unknown->name << NATIVE_TEXT ("\n");
        }
        return {sw, EXIT_FAILURE};
    }

    if (parse.nonOptionsCount () != 2) {
        error_stream << NATIVE_TEXT (USAGE) << std::endl;
        return {sw, EXIT_FAILURE};
    }

    sw.db_path = pstore::utf::from_native_string (parse.nonOption (0));
    sw.key = pstore::utf::from_native_string (parse.nonOption (1));

    sw.revision = pstore::head_revision;
    if (auto const & option = options[revision_opt]) {
        unsigned rev;
        bool is_valid;
        std::tie (rev, is_valid) =
            pstore::str_to_revision (pstore::utf::from_native_string (option.arg));
        assert (is_valid);
        sw.revision = rev;
    }

    sw.string_mode = (static_cast<option::Option const *> (options[string_opt]) != nullptr);

    return {sw, EXIT_SUCCESS};
}
#endif // PSTORE_IS_INSIDE_LLVM
// eof: tools/read/switches.cpp
