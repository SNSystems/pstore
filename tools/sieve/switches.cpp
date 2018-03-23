//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/sieve/switches.cpp -------------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

#if PSTORE_IS_INSIDE_LLVM
#include "llvm/Support/CommandLine.h"
using namespace llvm;
#else
#include "pstore/cmd_util/cl/command_line.hpp"
using namespace pstore::cmd_util;
#endif
#include "pstore/support/utf.hpp"

#undef output

namespace {

    cl::opt<switches::endian> endian_opt (
        "endian", cl::desc ("The endian-ness of the output data"),
        cl::values (cl::OptionEnumValue{"big", static_cast<int> (switches::endian::big),
                                        "Big-endian"},
                    cl::OptionEnumValue{"little", static_cast<int> (switches::endian::little),
                                        "Little-endian"},
                    cl::OptionEnumValue{"native", static_cast<int> (switches::endian::native),
                                        "The endian-ness of the host machine"}),
        cl::init (switches::endian::native));
    cl::alias endian2_opt ("e", cl::desc ("Alias for --endian"), cl::aliasopt (endian_opt));


    cl::opt<unsigned> maximum_opt ("maximum", cl::desc ("The maximum prime value"), cl::init (100));
    cl::alias maximum2_opt ("m", cl::desc ("Alias for --maximum"), cl::aliasopt (maximum_opt));


    cl::opt<std::string> output_opt ("output",
                                     cl::desc ("Output file name. (Default: standard-out)"),
                                     cl::init ("-"));
    cl::alias output2_opt ("o", cl::desc ("Alias for --output"), cl::aliasopt (output_opt));

} // anonymous namespace

namespace switches {

    user_options user_options::get (int argc, pstore_tchar * argv[]) {
        cl::ParseCommandLineOptions (argc, argv, "pstore prime number generator\n");

        user_options Result;
        Result.output =
            std::make_shared<std::string> (pstore::utf::from_native_string (output_opt));
        Result.endianness = endian_opt;
        Result.maximum = maximum_opt;
        return Result;
    }

} // namespace switches

// eof: tools/sieve/switches.cpp
