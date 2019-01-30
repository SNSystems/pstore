//*                                        __      *
//*   __ _  ___ _ __  _ __ ___  _ __ ___  / _|___  *
//*  / _` |/ _ \ '_ \| '__/ _ \| '_ ` _ \| |_/ __| *
//* | (_| |  __/ | | | | | (_) | | | | | |  _\__ \ *
//*  \__, |\___|_| |_|_|  \___/|_| |_| |_|_| |___/ *
//*  |___/                                         *
//===- tools/genromfs/genromfs.cpp ----------------------------------------===//
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
#ifdef _WIN32
#    define NO_MIN_MAX
#    define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_set>

#ifdef _WIN32
#    include <tchar.h>
#endif

#include "pstore/cmd_util/cl/command_line.hpp"
#include "pstore/support/make_unique.hpp"
#include "pstore/support/portab.hpp"

#include "copy.hpp"
#include "dump_tree.hpp"
#include "scan.hpp"

namespace {

    using namespace pstore::cmd_util;
    cl::opt<std::string> src_path (cl::Positional, ".", cl::desc ("source-path"));

} // end anonymous namespace

#ifdef _WIN32
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    cl::ParseCommandLineOptions (argc, argv, "pstore romfs generation utility\n");
    int exit_code = EXIT_SUCCESS;

    PSTORE_TRY {
        std::cout << "#include \"pstore/romfs/romfs.hpp\"\n";
        auto root = std::make_unique<directory_container> ();
        auto root_id = scan (*root, src_path, 0);
        std::unordered_set<unsigned> forwards;
        dump_tree (std::cout, forwards, *root, root_id, root_id);
        std::cout << "romfs::romfs fs (&dir" << root_id << ");\n";
    }
    PSTORE_CATCH (std::exception const & ex, {
        std::cerr << "Error: " << ex.what () << '\n';
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        std::cerr << "An unknown error occurred\n";
        exit_code = EXIT_FAILURE;
    })
    return exit_code;
}