//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/dump/switches.cpp --------------------------------------------===//
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
// 3rd-party
#include "optionparser.h"
//pstore
#include "pstore_cmd_util/str_to_revision.hpp"
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
        all_opt,
        contents_opt,
        fragments_opt,
		tickets_opt,
        header_opt,
        indices_opt,
        log_opt,
        shared_opt,
        no_times_opt,
        hex_opt,
        expanded_addresses_opt,
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

    option::Descriptor const usage[] = {
        {unknown_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT (""), option::Arg::None,
         NATIVE_TEXT ("Usage: dump [options] [data files]\n\nOptions:")},
        {help_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT ("help"), option::Arg::None,
         NATIVE_TEXT ("  --help  \tPrint usage and exit.")},

        {contents_opt, 0, NATIVE_TEXT ("c"), NATIVE_TEXT ("contents"), option::Arg::None,
         NATIVE_TEXT ("  --contents, -c  \tContents.")},
        {fragments_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT ("fragments"), option::Arg::None,
         NATIVE_TEXT ("  --fragments     \tDump the contents of the fragments index.")},
        {tickets_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT ("tickets"), option::Arg::None,
		 NATIVE_TEXT ("  --tickets       \tDump the contents of the tickets index.") },
        {header_opt, 0, NATIVE_TEXT ("h"), NATIVE_TEXT ("header"), option::Arg::None,
         NATIVE_TEXT ("  --header, -h  \tDump the file header.")},
        {indices_opt, 0, NATIVE_TEXT ("i"), NATIVE_TEXT ("indices"), option::Arg::None,
         NATIVE_TEXT ("  --indices, -i  \tUmm, list the indices.")},
        {log_opt, 0, NATIVE_TEXT ("l"), NATIVE_TEXT ("log"), option::Arg::None,
         NATIVE_TEXT ("  --log, -l  \tUmm, list the generations.")},

        {all_opt, 0, NATIVE_TEXT ("a"), NATIVE_TEXT ("all"), option::Arg::None,
         NATIVE_TEXT (
             "  --all, -a  \t"
             "Show store-related output. Equivalent to: --contents --header --indices --log.")},

        {shared_opt, 0, NATIVE_TEXT ("s"), NATIVE_TEXT ("shared-memory"), option::Arg::None,
         NATIVE_TEXT ("  --shared-memory, -s  \tThe shared-memory block.")},
        {revision_opt, 0, NATIVE_TEXT ("r"), NATIVE_TEXT ("revision"), revision,
         NATIVE_TEXT ("  --revision, -r  \tThe starting revision number (or 'HEAD').")},
        {no_times_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT ("no-times"), option::Arg::None,
         NATIVE_TEXT ("  --no-times \ttimes are displayed as a fixed value.")},
        {hex_opt, 0, NATIVE_TEXT ("x"), NATIVE_TEXT ("hex"), option::Arg::None,
         NATIVE_TEXT ("  --hex, -x  \tEmit numeric values in hexademical notation.")},
        {expanded_addresses_opt, 0, NATIVE_TEXT(""), NATIVE_TEXT ("expanded-addresses"), option::Arg::None,
         NATIVE_TEXT ("  --expanded-addresses  \tEmit addresses as explicit segment/offset objects.")},

        {unknown_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT (""), option::Arg::None,
         NATIVE_TEXT ("\nExamples:\n"
                      "  dump -h db.db\n"
                      "  dump -unk --plus -ppp file1 file2\n")},
        {0, 0, 0, 0, 0, 0},
    };

} // end of anonymous namespace


std::pair <switches, int> get_switches (int argc, TCHAR * argv []) {
   // Skip program name argv[0] if present
    argc -= (argc > 0);
    argv += (argc > 0);

    option::Stats stats (usage, argc, argv);
    std::vector<option::Option> options (stats.options_max);
    std::vector<option::Option> buffer (stats.buffer_max);
    option::Parser parse (usage, argc, argv, options.data (), buffer.data ());
    if (parse.error ()) {
        return {{}, EXIT_FAILURE};
    }

    if (options[help_opt] || argc == 0) {
        option::printUsage (out_stream, usage);
        return {{}, EXIT_FAILURE};
    }

    if (option::Option const * unknown = options[unknown_opt]) {
        for (; unknown != nullptr; unknown = unknown->next ()) {
            out_stream << NATIVE_TEXT ("Unknown option: ") << unknown->name
                       << NATIVE_TEXT ("\n");
        }
        return {{}, EXIT_FAILURE};
    }

    switches result;
    result.show_contents = options[contents_opt] != nullptr;
    result.show_fragments = options[fragments_opt] != nullptr;
	result.show_tickets = options[tickets_opt] != nullptr;
    result.show_header = options[header_opt] != nullptr;
    result.show_indices = options[indices_opt] != nullptr;
    result.show_log = options[log_opt] != nullptr;
    result.show_shared = options[shared_opt] != nullptr;
    result.show_all = options[all_opt] != nullptr;

    result.hex = options[hex_opt] != nullptr;
    result.no_times = options[no_times_opt] != nullptr;
    result.expanded_addresses = options[expanded_addresses_opt] != nullptr;

    result.revision = pstore::head_revision;
    if (auto const & option = options[revision_opt]) {
        unsigned rev;
        bool is_valid;
        std::tie (rev, is_valid) =
            pstore::str_to_revision (pstore::utf::from_native_string (option.arg));
        assert (is_valid);
        result.revision = rev;
    }


    TCHAR const ** path = parse.nonOptions ();
    std::for_each (path, path + parse.nonOptionsCount(), [&result] (TCHAR const * p) {
        result.paths.emplace_back (pstore::utf::from_native_string (p));
    });
    
    return {result, EXIT_SUCCESS};
}

#endif //!PSTORE_IS_INSIDE_LLVM

// eof: tools/dump/switches.cpp
