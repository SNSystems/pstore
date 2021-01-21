//*                          *
//*   ___  _ __   ___ _ __   *
//*  / _ \| '_ \ / _ \ '_ \  *
//* | (_) | |_) |  __/ | | | *
//*  \___/| .__/ \___|_| |_| *
//*       |_|                *
//===- unittests/romfs/klee/romfs/open.cpp --------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
