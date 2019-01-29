#include <cassert>
#include <klee/klee.h>
#include "pstore/support/uint128.hpp"
#include "./common.hpp"

int main (int argc, char * argv[]) {
    pstore::uint128 value;
    klee_make_symbolic (&value, sizeof (value), "value");
    klee_assume (to_native (value) > 0U);

    __uint128_t valuex = to_native (value);

#if PSTORE_KLEE_RUN
    dump_uint128 ("before:", value);
#endif

    value--;
    valuex--;

#if PSTORE_KLEE_RUN
    fputc (' ', stdout);
    dump_uint128 ("after:", value);
#endif

    assert (value == valuex);
}
