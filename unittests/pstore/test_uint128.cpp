//*        _       _   _ ____  ___   *
//*  _   _(_)_ __ | |_/ |___ \( _ )  *
//* | | | | | '_ \| __| | __) / _ \  *
//* | |_| | | | | | |_| |/ __/ (_) | *
//*  \__,_|_|_| |_|\__|_|_____\___/  *
//*                                  *
//===- unittests/pstore/test_uint128.cpp ----------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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

#include "pstore/uint128.hpp"
#include <gtest/gtest.h>

TEST (UInt128, DefaultCtor) {
    pstore::uint128 v;
    EXPECT_EQ (v.high (), 0U);
    EXPECT_EQ (v.low (), 0U);
}

TEST (UInt128, ExplicitCtor) {
    constexpr auto high = std::uint64_t{7};
    constexpr auto low = std::uint64_t{5};
    {
        pstore::uint128 v1{high, low};
        EXPECT_EQ (v1.high (), high);
        EXPECT_EQ (v1.low (), low);
    }
    {
        pstore::uint128 v2{low};
        EXPECT_EQ (v2.high (), 0U);
        EXPECT_EQ (v2.low (), low);
    }
}

TEST (UInt128, Equal) {
    constexpr auto high = std::uint64_t{7};
    constexpr auto low = std::uint64_t{5};

    pstore::uint128 v1{high, low};
    pstore::uint128 v2{high, low};
    pstore::uint128 v3{high, low + 1};
    pstore::uint128 v4{high + 1, low};

    EXPECT_TRUE (v1 == v2);
    EXPECT_FALSE (v1 != v2);
    EXPECT_FALSE (v1 == v3);
    EXPECT_TRUE (v1 != v3);
    EXPECT_FALSE (v1 == v4);
    EXPECT_TRUE (v1 != v4);

    EXPECT_TRUE (pstore::uint128{5U} == 5U);
    EXPECT_TRUE (pstore::uint128{5U} != 6U);
}

TEST (UInt128, Gt) {
    EXPECT_TRUE (pstore::uint128 (0, 1) > pstore::uint128 (0, 0));
    EXPECT_TRUE (pstore::uint128 (0, 1) >= pstore::uint128 (0, 0));
    EXPECT_TRUE (pstore::uint128 (2, 1) > pstore::uint128 (1, 2));
    EXPECT_TRUE (pstore::uint128 (2, 1) >= pstore::uint128 (1, 2));
    EXPECT_TRUE (pstore::uint128 (1, 1) >= pstore::uint128 (1, 1));

    EXPECT_TRUE (pstore::uint128{6U} > 5U);
    EXPECT_TRUE (pstore::uint128{6U} >= 6U);
}

TEST (UInt128, Lt) {
    EXPECT_TRUE (pstore::uint128 (0, 0) < pstore::uint128 (0, 1));
    EXPECT_TRUE (pstore::uint128 (0, 0) <= pstore::uint128 (0, 1));
    EXPECT_TRUE (pstore::uint128 (1, 2) < pstore::uint128 (2, 1));
    EXPECT_TRUE (pstore::uint128 (1, 2) <= pstore::uint128 (2, 1));
    EXPECT_TRUE (pstore::uint128 (1, 1) <= pstore::uint128 (1, 1));
}
// eof: unittests/pstore/test_uint128.cpp
