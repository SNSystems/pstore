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
