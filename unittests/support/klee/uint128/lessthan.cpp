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

    bool const comp = lhs < rhs;
    bool const compx = to_native (lhs) < to_native (rhs);

    // std::printf ("value:0x%" PRIx64 ",0x%" PRIx64 " dist:0x%x\n", value.high(), value.low (),
    // dist);
    assert (comp == compx);
}
