//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/write/switches.cpp -------------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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

#include "pstore/cmd_util/command_line.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/utf.hpp"

#include "error.hpp"
#include "to_value_pair.hpp"

using namespace pstore::cmd_util;

namespace {

    cl::list<std::string>
        add ("add", cl::desc ("Add key with corresponding string value. Specified as 'key,value'."
                              " May be repeated to add several keys."));
    cl::alias add2 ("a", cl::desc ("Alias for --add"), cl::aliasopt (add));

    cl::list<std::string>
        add_string ("add-string",
                    cl::desc ("Add key to string set. May be repeated to add several strings."));
    cl::alias add_string2 ("s", cl::desc ("Alias for --add-string"), cl::aliasopt (add_string));

    cl::list<std::string>
        add_file ("add-file",
                  cl::desc ("Add key with the named file's contents as the corresponding value."
                            " Specified as 'key,filename'. May be repeated to add several files."));
    cl::alias add_file2 ("f", cl::desc ("Alias for --add-file"), cl::aliasopt (add_file));


    cl::opt<std::string> db_path (cl::positional,
                                  cl::desc ("<Path of the pstore repository to be written>"),
                                  cl::required);
    cl::list<std::string> files (cl::positional, cl::desc ("<filename>..."));

    cl::opt<std::string> vacuum_mode ("compact", cl::optional,
                                      cl::desc ("Set the compaction mode. Argument must one of: "
                                                "'disabled', 'immediate', 'background'."));
    cl::alias vacuum_mode2 ("c", cl::desc ("Alias for --compact"), cl::aliasopt (vacuum_mode));

    pstore::database::vacuum_mode to_vacuum_mode (std::string const & opt) {
        if (opt == "disabled") {
            return pstore::database::vacuum_mode::disabled;
        } else if (opt == "immediate") {
            return pstore::database::vacuum_mode::immediate;
        } else if (opt == "background") {
            return pstore::database::vacuum_mode::background;
        }

        pstore::raise_error_code (make_error_code (write_error_code::unrecognized_compaction_mode));
    }

} // end anonymous namespace

std::pair<switches, int> get_switches (int argc, tchar * argv[]) {
    cl::ParseCommandLineOptions (argc, argv, "pstore write utility\n");

    auto const make_value_pair = [](std::string const & arg) { return to_value_pair (arg); };

    switches result;

    result.db_path = db_path.get ();
    if (!vacuum_mode.empty ()) {
        result.vmode = to_vacuum_mode (vacuum_mode.get ());
    }

    std::transform (std::begin (add), std::end (add), std::back_inserter (result.add),
                    make_value_pair);
    result.strings = add_string.get ();
    std::transform (std::begin (add_file), std::end (add_file), std::back_inserter (result.files),
                    make_value_pair);
    std::transform (std::begin (files), std::end (files), std::back_inserter (result.files),
                    [](std::string const & path) { return std::make_pair (path, path); });

    return {result, EXIT_SUCCESS};
}
