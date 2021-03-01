//===- unittests/support/klee/uint128/postinc.cpp -------------------------===//
//*                  _   _             *
//*  _ __   ___  ___| |_(_)_ __   ___  *
//* | '_ \ / _ \/ __| __| | '_ \ / __| *
//* | |_) | (_) \__ \ |_| | | | | (__  *
//* | .__/ \___/|___/\__|_|_| |_|\___| *
//* |_|                                *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <cassert>
#include <klee/klee.h>
#include "pstore/support/uint128.hpp"
#include "./common.hpp"

int main (int argc, char * argv[]) {
    pstore::uint128 value;
    klee_make_symbolic (&value, sizeof (value), "value");
    klee_assume (to_native (value) < max64 ());

    __uint128_t valuex = to_native (value);

#if PSTORE_KLEE_RUN
    std::fputc ('\n', stdout);
    dump_uint128 ("before:", value);
    std::fputc ('\n', stdout);
#endif

    value++;
    valuex++;

#if PSTORE_KLEE_RUN
    dump_uint128 ("after:", value);
    std::fputc ('\n', stdout);
#endif

    assert (value == valuex);
}
