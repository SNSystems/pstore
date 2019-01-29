#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <string>

#include <klee/klee.h>

#include "pstore/support/path.hpp"

constexpr std::size_t size = 5;

int main (int argc, char * argv[]) {
    char str1[size];
    char str2[size];

    klee_make_symbolic (&str1, sizeof (str1), "str1");
    klee_assume (str1[size - 1] == '\0');

    klee_make_symbolic (&str2, sizeof (str2), "str2");
    klee_assume (str2[size - 1] == '\0');

    std::string resl = pstore::path::posix::join (str1, str2);
}
