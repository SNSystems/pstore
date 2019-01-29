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

    value >>= dist;

    __uint128_t valuex = to_native (value);
    valuex >>= dist;

#if 1 // PSTORE_KLEE_RUN
    fputs ("value:", stdout);
    dump_uint128 (value);
    std::printf (" dist:0x%x\n", dist);
#endif

    assert (value == valuex);
}
