//===- unittests/adt/klee/cv/resize.cpp -----------------------------------===//
//*                _          *
//*  _ __ ___  ___(_)_______  *
//* | '__/ _ \/ __| |_  / _ \ *
//* | | |  __/\__ \ |/ /  __/ *
//* |_|  \___||___/_/___\___| *
//*                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file resize.cpp
/// \brief KLEE test for chunked_vector<>::resize().
///
/// This test makes two calls to resize() both using symbolic arguments. The idea behind this is
/// that we start with an empty container, resize it and then resize it again. The second of these
/// resize operations may be a no-op or will enlarge or shrink the container from the starting point
/// produced by the first resize. This should enable the test to cover all of the possible start and
/// end conditions.

#include <algorithm>
#include <cassert>
#include <iterator>

#include <klee/klee.h>

#include "pstore/adt/chunked_vector.hpp"

namespace {

    constexpr auto elements_per_chunk = 3U;
    constexpr auto max_size = elements_per_chunk * 3U;

    void check (pstore::chunked_vector<int, elements_per_chunk> const & cv,
                std::size_t const size) {
        assert (cv.size () == size);
        assert (std::distance (std::begin (cv), std::end (cv)) == size);
        assert (std::all_of (std::begin (cv), std::end (cv), [] (int x) { return x == 0; }));
    }

} // end anonymous namespace


int main () {
    pstore::chunked_vector<int, elements_per_chunk> cv;
    std::size_t new_size1;
    std::size_t new_size2;
    klee_make_symbolic (&new_size1, sizeof (new_size1), "new_size1");
    klee_make_symbolic (&new_size2, sizeof (new_size2), "new_size2");
    // Limit the size to < max_size since this should represent more than enough test cases to
    // exercise every possible code path.
    klee_assume (new_size1 < max_size);
    klee_assume (new_size2 < max_size);

    cv.resize (new_size1);
    check (cv, new_size1);
    cv.resize (new_size2);
    check (cv, new_size2);
}
