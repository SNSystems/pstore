//===- unittests/support/klee/uint128/add.cpp -----------------------------===//
//*            _     _  *
//*   __ _  __| | __| | *
//*  / _` |/ _` |/ _` | *
//* | (_| | (_| | (_| | *
//*  \__,_|\__,_|\__,_| *
//*                     *
//===----------------------------------------------------------------------===//
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
#include <klee/klee.h>
#include "pstore/support/uint128.hpp"
#include "./common.hpp"

int main (int argc, char * argv[]) {
    pstore::uint128 lhs;
    pstore::uint128 rhs;
    klee_make_symbolic (&lhs, sizeof (lhs), "lhs");
    klee_make_symbolic (&rhs, sizeof (rhs), "rhs");

    __uint128_t lhsx = to_native (lhs);
    __uint128_t const rhsx = to_native (rhs);

#if PSTORE_KLEE_RUN
    dump_uint128 ("before lhs:", lhs, "rhs:", rhs);
    dump_uint128 ("before lhsx:", lhsx, "rhsx:", rhsx);
#endif

    lhs += rhs;
    lhsx += rhsx;

#if PSTORE_KLEE_RUN
    dump_uint128 ("after lhs:", lhs, "rhs:", rhs);
    dump_uint128 ("after lhsx:", lhsx, "rhsx:", rhsx);
#endif

    assert (lhs == lhsx);
}
