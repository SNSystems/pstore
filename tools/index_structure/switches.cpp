//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/index_structure/switches.cpp ---------------------------------===//
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

//3rd-party
#include "optionparser.h"

// pstore
#include "pstore_cmd_util/str_to_revision.hpp"
#include "pstore_cmd_util/revision_opt.hpp"
#include "pstore_support/utf.hpp"


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
    };

    option::ArgStatus revision (option::Option const & option, bool msg) {
        auto r = pstore::str_to_revision (pstore::utf::from_native_string (option.arg));
        if (r.second) {
            return option::ARG_OK;
        }
        if (msg) {
            error_stream << "Option '" << option.name
                         << "' requires an argument which may be a revision number or 'HEAD'";
            if (option.arg) {
                error_stream << " (got '" << option.arg << "')";
            }
            error_stream << '\n';
        }
        return option::ARG_ILLEGAL;
    }


    option::Descriptor usage[] = {
        {unknown_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT (""), option::Arg::None,
         NATIVE_TEXT ("Usage: pstore_index_structure [options] database-path <index-name>...\n\nOptions:")},
        {help_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT ("help"), option::Arg::None,
         NATIVE_TEXT ("  --help        \tPrint usage and exit.")},
        {revision_opt, 0, NATIVE_TEXT ("r"), NATIVE_TEXT ("revision"), revision,
         NATIVE_TEXT ("  --revision, -r  \tThe starting revision number (or 'HEAD').")},
        {0, 0, 0, 0, 0, 0},
    };

    std::string usage_help () {
        std::ostringstream os;
        os << "USAGE: pstore_index_struct [options] database-path <index-name>...\n\n"
           << "Dumps the internal structure of one of more pstore indexes. index-name may be any of: ";
        char const * separator = "";
        for (auto const & in: index_names) {
            os << separator << '\'' << in << '\'';
            separator = ", ";
        }
        os << " ('*' may be used as a shortcut for all names).\n"
           << "\nOptions:\n";
        return os.str ();
     }

} // (anonymous namespace)

std::pair<switches, int> get_switches (int argc, pstore_tchar * argv []) {
    switches sw;

    argc -= (argc > 0);
    argv += (argc > 0);

    auto const usage_str = pstore::utf::to_native_string (usage_help ());
    usage [0].help = usage_str.c_str ();

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

    if (option::Option const * unknown = options[unknown_opt]) {
        for (; unknown != nullptr; unknown = unknown->next ()) {
            out_stream << NATIVE_TEXT ("Unknown option: ")
                       << unknown->name
                       << NATIVE_TEXT ("\n");
        }
        return {{}, EXIT_FAILURE};
    }

    if (auto const & option = options[revision_opt]) {
        unsigned rev;
        bool is_valid;
        std::tie (rev, is_valid) =
            pstore::str_to_revision (pstore::utf::from_native_string (option.arg));
        assert (is_valid);
        sw.revision = rev;
    }

    sw.db_path = pstore::utf::from_native_string (parse.nonOption (0));
    for (int count = 1, non_options_count = parse.nonOptionsCount (); count < non_options_count; ++count) {
        std::string const name = pstore::utf::from_native_string (parse.nonOption (count));
        if (!set_from_name (&sw.selected, name)) {
            error_stream << NATIVE_TEXT ("Unknown index \"") << pstore::utf::to_native_string (name) << NATIVE_TEXT ("\"\n");
            return {sw, EXIT_FAILURE};
        }
    }

    return {sw, EXIT_SUCCESS};
}
#endif //PSTORE_IS_INSIDE_LLVM
// eof: tools/index_structure/switches.cpp
