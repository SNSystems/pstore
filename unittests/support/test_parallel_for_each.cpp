//===- unittests/support/test_parallel_for_each.cpp -----------------------===//
//*                        _ _      _    __                              _      *
//*  _ __   __ _ _ __ __ _| | | ___| |  / _| ___  _ __    ___  __ _  ___| |__   *
//* | '_ \ / _` | '__/ _` | | |/ _ \ | | |_ / _ \| '__|  / _ \/ _` |/ __| '_ \  *
//* | |_) | (_| | | | (_| | | |  __/ | |  _| (_) | |    |  __/ (_| | (__| | | | *
//* | .__/ \__,_|_|  \__,_|_|_|\___|_| |_|  \___/|_|     \___|\__,_|\___|_| |_| *
//* |_|                                                                         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/support/parallel_for_each.hpp"

// Standard library includes
#include <algorithm>
#include <array>
#include <mutex>
#include <vector>

// 3rd party includes
#include <gmock/gmock.h>

namespace {

    class ParallelForEach : public ::testing::Test {
    protected:
        using container = std::vector<int>;

        static unsigned concurrency () {
            return std::max (std::thread::hardware_concurrency (), 1U);
        }

        static container make_input (unsigned num) {
            return build_vector (num, [] (int c) { return c; });
        }

        static container make_expected (unsigned num) {
            return build_vector (num, [] (int c) { return c * 2; });
        }

        static container run_for_each (container const & src);

    private:
        template <typename UnaryFunction>
        static container build_vector (unsigned num, UnaryFunction f);
    };

    // run_for_each
    // ~~~~~~~~~~~~
    auto ParallelForEach::run_for_each (container const & src) -> container {
        std::mutex mut;
        container out;
        pstore::parallel_for_each (std::begin (src), std::end (src), [&out, &mut] (int v) {
            std::lock_guard<std::mutex> _{mut};
            out.emplace_back (v * 2);
        });
        std::sort (std::begin (out), std::end (out), std::less<int> ());
        return out;
    }

    // build_vector
    // ~~~~~~~~~~~~
    template <typename UnaryFunction>
    auto ParallelForEach::build_vector (unsigned num, UnaryFunction f) -> container {
        container v;
        auto count = 0;
        std::generate_n (std::back_inserter (v), num, [&count, f] () { return f (++count); });
        return v;
    }

} // end anonymous namespace

TEST_F (ParallelForEach, ZeroElements) {
    auto const out = run_for_each (container{});
    EXPECT_THAT (out, ::testing::ContainerEq (container{}));
}

TEST_F (ParallelForEach, OneElement) {
    auto const src = make_input (1U);
    auto const expected = make_expected (1U);
    auto const out = run_for_each (src);
    EXPECT_THAT (out, ::testing::ContainerEq (expected));
}

TEST_F (ParallelForEach, ConcurrencyMinusOne) {
    auto num = concurrency () - 1U;
    auto const src = make_input (num);
    auto const expected = make_expected (num);
    auto const out = run_for_each (src);
    EXPECT_THAT (out, ::testing::ContainerEq (expected));
}

TEST_F (ParallelForEach, Concurrency) {
    auto num = concurrency ();
    auto const src = make_input (num);
    auto const expected = make_expected (num);
    auto const out = run_for_each (src);
    EXPECT_THAT (out, ::testing::ContainerEq (expected));
}

TEST_F (ParallelForEach, ConcurrencyPlusOne) {
    auto num = concurrency () + 1U;
    auto const src = make_input (num);
    auto const expected = make_expected (num);
    auto const out = run_for_each (src);
    EXPECT_THAT (out, ::testing::ContainerEq (expected));
}

TEST (ParallelForEachException, WorkerExceptionPropogates) {
    // Check that an exception throw in a worker thread fully propogates back to the caller.
#ifdef PSTORE_EXCEPTIONS
    class custom_exception : public std::exception {};
    auto op = [&] () {
        std::array<int, 2> const src{{3, 5}};
        pstore::parallel_for_each (std::begin (src), std::end (src),
                                   [] (int) { throw custom_exception{}; });
    };
    EXPECT_THROW (op (), custom_exception);
#endif // PSTORE_EXCEPTIONS
}
