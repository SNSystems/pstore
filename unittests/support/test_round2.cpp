//*                            _ ____   *
//*  _ __ ___  _   _ _ __   __| |___ \  *
//* | '__/ _ \| | | | '_ \ / _` | __) | *
//* | | | (_) | |_| | | | | (_| |/ __/  *
//* |_|  \___/ \__,_|_| |_|\__,_|_____| *
//*                                     *
//===- unittests/support/test_round2.cpp ----------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
