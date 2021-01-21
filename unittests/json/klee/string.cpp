//*      _        _              *
//*  ___| |_ _ __(_)_ __   __ _  *
//* / __| __| '__| | '_ \ / _` | *
//* \__ \ |_| |  | | | | | (_| | *
//* |___/\__|_|  |_|_| |_|\__, | *
//*                       |___/  *
//===- unittests/json/klee/string.cpp -------------------------------------===//
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

#include "pstore/json/utility.hpp"

int main (int argc, char * argv[]) {
    constexpr auto buffer_size = 7U;
    char buffer[buffer_size];
    klee_make_symbolic (&buffer, sizeof (buffer), "buffer");
    klee_assume (buffer[0] == '"');
    klee_assume (buffer[buffer_size - 1U] == '\0');

#ifdef PSTORE_KLEE_RUN
    std::cout << buffer << '\n';
#endif

    pstore::json::is_valid (buffer);
}
