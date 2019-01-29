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
