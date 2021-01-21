//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/sieve/switches.cpp -------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#include "switches.hpp"

#include "pstore/command_line/command_line.hpp"

using namespace pstore::command_line;

namespace {

    opt<endian>
        endian_opt ("endian", desc ("The endian-ness of the output data"),
                    values (literal{"big", static_cast<int> (endian::big), "Big-endian"},
                            literal{"little", static_cast<int> (endian::little), "Little-endian"},
                            literal{"native", static_cast<int> (endian::native),
                                    "The endian-ness of the host machine"}),
                    init (endian::native));
    alias endian2_opt ("e", desc ("Alias for --endian"), aliasopt (endian_opt));


    opt<unsigned> maximum_opt ("maximum", desc ("The maximum prime value"), init (100U));
    alias maximum2_opt ("m", desc ("Alias for --maximum"), aliasopt (maximum_opt));


    opt<std::string> output_opt ("output", desc ("Output file name. (Default: standard-out)"),
                                 init ("-"));
    alias output2_opt ("o", desc ("Alias for --output"), aliasopt (output_opt));

} // end anonymous namespace

user_options user_options::get (int argc, tchar * argv[]) {
    parse_command_line_options (argc, argv, "pstore prime number generator\n");

    user_options result;
    result.output = output_opt.get ();
    result.endianness = endian_opt.get ();
    result.maximum = maximum_opt.get ();
    return result;
}
