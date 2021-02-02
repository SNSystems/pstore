//*    _       _        *
//*   (_) ___ (_)_ __   *
//*   | |/ _ \| | '_ \  *
//*   | | (_) | | | | | *
//*  _/ |\___/|_|_| |_| *
//* |__/                *
//===- unittests/os/klee/path/join.cpp ------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <string>

#include <klee/klee.h>

#include "pstore/os/path.hpp"

constexpr std::size_t size = 5;

int main (int argc, char * argv[]) {
    char str1[size];
    char str2[size];

    klee_make_symbolic (&str1, sizeof (str1), "str1");
    klee_assume (str1[size - 1] == '\0');

    klee_make_symbolic (&str2, sizeof (str2), "str2");
    klee_assume (str2[size - 1] == '\0');

    std::string resl = pstore::path::posix::join (str1, str2);
}
