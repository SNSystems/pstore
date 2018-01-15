//*  _ _                                _ _       _                *
//* | | |_   ___ __ ___    _____      _(_) |_ ___| |__   ___  ___  *
//* | | \ \ / / '_ ` _ \  / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* | | |\ V /| | | | | | \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |_|_| \_/ |_| |_| |_| |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                                                *
//===- tools/sieve/llvm_switches.cpp --------------------------------------===//
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

#if PSTORE_IS_INSIDE_LLVM
#include "llvm/Support/CommandLine.h"
#else
#include "pstore_cmd_util/cl/command_line.hpp"
#endif
#include "pstore_support/utf.hpp"

#if PSTORE_IS_INSIDE_LLVM
    using namespace llvm;
#else
    using namespace pstore::cmd_util;
#endif

namespace {

    cl::opt<switches::endian> Endian (
        "endian", cl::desc ("The endian-ness of the output data"),
        cl::values (cl::OptionEnumValue{"big", static_cast <int> (switches::endian::big), "Big-endian"},
                    cl::OptionEnumValue{"little", static_cast <int> (switches::endian::little), "Little-endian"},
                    cl::OptionEnumValue{"native", static_cast <int> (switches::endian::native),
                                        "The endian-ness of the host machine"}),
        cl::init (switches::endian::native));
    cl::alias Endian2 ("e", cl::desc ("Alias for --endian"), cl::aliasopt (Endian));


    cl::opt<unsigned> Maximum ("maximum", cl::desc ("The maximum prime value"),
                               cl::init (100) // FIXME: duplicated in switches.cpp
                               );
    cl::alias Maximum2 ("m", cl::desc ("Alias for --maximum"), cl::aliasopt (Maximum));


    cl::opt<std::string> Output ("output", cl::desc ("Output file name. (Default: standard-out)"),
                                 cl::init ("-"));
    cl::alias Output2 ("o", cl::desc ("Alias for --output"), cl::aliasopt (Output));

} // anonymous namespace

namespace switches {

    user_options user_options::get (int argc, char * argv[]) {
        cl::ParseCommandLineOptions (argc, argv, "pstore prime number generator\n");

        user_options Result;
        Result.output = std::make_shared<std::string> (pstore::utf::from_native_string (Output));
        Result.endianness = Endian;
        Result.maximum = Maximum;
        return Result;
    }

} // namespace switches

// eof: tools/sieve/llvm_switches.cpp
