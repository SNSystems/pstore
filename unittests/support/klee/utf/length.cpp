//*  _                  _   _      *
//* | | ___ _ __   __ _| |_| |__   *
//* | |/ _ \ '_ \ / _` | __| '_ \  *
//* | |  __/ | | | (_| | |_| | | | *
//* |_|\___|_| |_|\__, |\__|_| |_| *
//*               |___/            *
//===- unittests/support/klee/utf/length.cpp ------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <array>
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include <klee/klee.h>

#include "pstore/support/utf.hpp"

constexpr auto size = std::size_t{5};

int main (int argc, char * argv[]) {
    char str[size];
    // std::memset (str, 0, sizeof (str));

    klee_make_symbolic (&str, sizeof (str), "str");
    klee_assume (str[size - 1] == '\0');

    char const * cstr = str;
    std::size_t l = pstore::utf::length (cstr);
    assert (l < size);
}
