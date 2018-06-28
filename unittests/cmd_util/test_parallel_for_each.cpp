//*                        _ _      _    __                              _      *
//*  _ __   __ _ _ __ __ _| | | ___| |  / _| ___  _ __    ___  __ _  ___| |__   *
//* | '_ \ / _` | '__/ _` | | |/ _ \ | | |_ / _ \| '__|  / _ \/ _` |/ __| '_ \  *
//* | |_) | (_| | | | (_| | | |  __/ | |  _| (_) | |    |  __/ (_| | (__| | | | *
//* | .__/ \__,_|_|  \__,_|_|_|\___|_| |_|  \___/|_|     \___|\__,_|\___|_| |_| *
//* |_|                                                                         *
//===- unittests/pstore_cmd_util/test_parallel_for_each.cpp ---------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
//===----------------------------------------------------------------------===//

#include "pstore/cmd_util/parallel_for_each.hpp"

#include <algorithm>
#include <array>
#include <mutex>
#include <vector>

#include "gmock/gmock.h"

namespace {
    class ParallelForEach : public ::testing::Test {
    protected:
        using container = std::vector<int>;

        static unsigned concurrency () {
            return std::max (std::thread::hardware_concurrency (), 1U);
        }

        static container make_input (unsigned num) {
            return build_vector (num, [](int c) { return c; });
        }

        static container make_expected (unsigned num) {
            return build_vector (num, [](int c) { return c * 2; });
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
        pstore::cmd_util::parallel_for_each (std::begin (src), std::end (src), [&out, &mut](int v) {
            std::lock_guard<std::mutex> lock (mut);
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
        std::generate_n (std::back_inserter (v), num, [&count, f]() { return f (++count); });
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
// eof: unittests/pstore_cmd_util/test_parallel_for_each.cpp
