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
