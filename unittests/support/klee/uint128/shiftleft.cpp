//*      _     _  __ _   _       __ _    *
//*  ___| |__ (_)/ _| |_| | ___ / _| |_  *
//* / __| '_ \| | |_| __| |/ _ \ |_| __| *
//* \__ \ | | | |  _| |_| |  __/  _| |_  *
//* |___/_| |_|_|_|  \__|_|\___|_|  \__| *
//*                                      *
//===- unittests/support/klee/uint128/shiftleft.cpp -----------------------===//
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
    unsigned dist;
    klee_make_symbolic (&value, sizeof (value), "value");
    klee_make_symbolic (&dist, sizeof (dist), "dist");
    klee_assume (dist < 128);

    pstore::uint128 const result = value << dist;
    __uint128_t const resultx = to_native (value) << dist;

#if 1 // PSTORE_KLEE_RUN
    std::fputs ("value:", stdout);
    dump_uint128 ("value:", value);
    std::printf (" dist:0x%x\n", dist);
#endif

    assert (result == resultx);
}
