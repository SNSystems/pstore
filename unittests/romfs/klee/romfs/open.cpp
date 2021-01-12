//*                          *
//*   ___  _ __   ___ _ __   *
//*  / _ \| '_ \ / _ \ '_ \  *
//* | (_) | |_) |  __/ | | | *
//*  \___/| .__/ \___|_| |_| *
//*       |_|                *
//===- unittests/romfs/klee/romfs/open.cpp --------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
#include <cassert>
#include <cinttypes>
#include <cstdio>

#ifdef PSTORE_KLEE_RUN
#    include <iostream>
#endif

#include <klee/klee.h>

#include "pstore/romfs/romfs.hpp"

using namespace pstore::romfs;

namespace {

    std::uint8_t const file1[] = {0};
    extern directory const dir0;
    extern directory const dir3;
    std::array<dirent, 3> const dir0_membs = {{
        {".", &dir0},
        {"..", &dir3},
        {"f", file1, pstore::romfs::stat{sizeof (file1), 0, pstore::romfs::mode_t::file}},
    }};
    directory const dir0{dir0_membs};
    std::array<dirent, 4> const dir3_membs = {{
        {".", &dir3},
        {"..", &dir3},
        {"d", &dir0},
        {"g", file1, pstore::romfs::stat{sizeof (file1), 0, pstore::romfs::mode_t::file}},
    }};
    directory const dir3{dir3_membs};
    directory const * const root = &dir3;

} // end anonymous namespace

int main (int argc, char * argv[]) {
    constexpr auto buffer_size = 7U;
    char path[buffer_size];
    klee_make_symbolic (&path, sizeof (path), "path");
    klee_assume (path[buffer_size - 1U] == '\0');

#ifdef PSTORE_KLEE_RUN
    std::cout << path << '\n';
#endif

    romfs fs (root);
    fs.chdir ("d");
    fs.open (path);
}
