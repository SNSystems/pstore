//*                            _ ____   *
//*  _ __ ___  _   _ _ __   __| |___ \  *
//* | '__/ _ \| | | | '_ \ / _` | __) | *
//* | | | (_) | |_| | | | | (_| |/ __/  *
//* |_|  \___/ \__,_|_| |_|\__,_|_____| *
//*                                     *
//===- unittests/support/test_round2.cpp ----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/support/round2.hpp"

#include <gtest/gtest.h>

namespace {

    template <typename T, unsigned Shift>
    struct checkn {
        void operator() () const {
            checkn<T, Shift - 1U>{}();

            constexpr auto v = T{1} << Shift;
            EXPECT_EQ (pstore::round_to_power_of_2 (T{v - 1}), v)
                << "(1<<" << Shift << ")-1 should become 1<<" << Shift;
            EXPECT_EQ (pstore::round_to_power_of_2 (T{v + 0}), v)
                << "(1<<" << Shift << ") should become 1<<" << Shift;
            EXPECT_EQ (pstore::round_to_power_of_2 (T{v + 1}), T{1} << (Shift + 1))
                << "(1<<" << Shift << ")+1 should become 1<<" << (Shift + 1);
        }
    };

    template <typename T>
    struct checkn<T, 0U> {
        void operator() () const {
            EXPECT_EQ (pstore::round_to_power_of_2 (T{1}), 1U) << "1 should become 1";
        }
    };

    template <typename T>
    struct checkn<T, 1U> {
        void operator() () const {
            checkn<T, 0U>{}();
            constexpr auto v = T{1} << 1;
            EXPECT_EQ (pstore::round_to_power_of_2 (T{v}), v) << "1<<1 should become 1<<1";
        }
    };

    template <typename T>
    void check_round () {
        constexpr unsigned shift = sizeof (T) * 8 - 1;
        checkn<T, shift - 1>{}();

        constexpr auto v = T{1} << shift;
        EXPECT_EQ (pstore::round_to_power_of_2 (T{v - 1}), v)
            << "(1<<" << shift << ")-1 should become 1<<" << shift;
        EXPECT_EQ (pstore::round_to_power_of_2 (T{v + 0}), v)
            << "(1<<" << shift << ") should become 1<<" << shift;
        EXPECT_EQ (pstore::round_to_power_of_2 (T{v + 1}), 0U)
            << "(1<<" << shift << ")+1 should become 0";
        EXPECT_EQ (pstore::round_to_power_of_2 (std::numeric_limits<T>::max ()), 0U)
            << "max should become 0";
    }

} // end anonymous namespace

TEST (Round2, Uint8) {
    check_round<std::uint8_t> ();
}
TEST (Round2, Uint16) {
    check_round<std::uint16_t> ();
}
TEST (Round2, Uint32) {
    check_round<std::uint32_t> ();
}
TEST (Round2, Uint64) {
    check_round<std::uint64_t> ();
}
