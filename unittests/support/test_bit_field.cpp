//*  _     _ _      __ _      _     _  *
//* | |__ (_) |_   / _(_) ___| | __| | *
//* | '_ \| | __| | |_| |/ _ \ |/ _` | *
//* | |_) | | |_  |  _| |  __/ | (_| | *
//* |_.__/|_|\__| |_| |_|\___|_|\__,_| *
//*                                    *
//===- unittests/support/test_bit_field.cpp -------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/support/bit_field.hpp"
#include <limits>
#include <gtest/gtest.h>
#include "pstore/support/bit_count.hpp"
#include "pstore/support/portab.hpp"

namespace {

    template <typename T>
    class BitFieldAssignment : public ::testing::Test {};

    template <typename Type, unsigned Index, unsigned Bits>
    struct param {
        using value_type = Type;
        using index = std::integral_constant<Type, Index>;
        using bits = std::integral_constant<Type, Bits>;
    };

    using assign_test_types = ::testing::Types<
        param<std::uint8_t, 0, 1>, // testing bits [0,1)
        param<std::uint8_t, 1, 1>, // testing bits [1,2)
        param<std::uint8_t, 7, 1>, // ... and so on.
        param<std::uint16_t, 8, 1>, param<std::uint16_t, 15, 1>, param<std::uint32_t, 16, 1>,
        param<std::uint32_t, 31, 1>, param<std::uint64_t, 32, 1>, param<std::uint64_t, 63, 1>,

        param<std::uint8_t, 0, 2>, param<std::uint8_t, 1, 2>, param<std::uint8_t, 6, 2>,
        param<std::uint16_t, 7, 2>, param<std::uint16_t, 8, 2>, param<std::uint16_t, 14, 2>,
        param<std::uint32_t, 15, 2>, param<std::uint32_t, 16, 2>, param<std::uint64_t, 31, 2>,
        param<std::uint64_t, 32, 2>, param<std::uint64_t, 62, 2>,

        param<std::uint8_t, 0, 7>, param<std::uint8_t, 0, 8>, param<std::uint16_t, 0, 9>,
        param<std::uint16_t, 0, 15>, param<std::uint16_t, 0, 16>, param<std::uint32_t, 0, 17>,
        param<std::uint32_t, 0, 31>, param<std::uint32_t, 0, 32>, param<std::uint64_t, 0, 63>,
        param<std::uint64_t, 0, 64>>;

} // end anonymous namespace

#ifdef PSTORE_IS_INSIDE_LLVM
TYPED_TEST_CASE (BitFieldAssignment, assign_test_types);
#else
TYPED_TEST_SUITE (BitFieldAssignment, assign_test_types, );
#endif // PSTORE_IS_INSIDE_LLVM

TYPED_TEST (BitFieldAssignment, Assignment) {
    using value_type = typename TypeParam::value_type;
    value_type const index = typename TypeParam::index ();
    value_type const bits = typename TypeParam::bits ();

    union {
        value_type vt;
        pstore::bit_field<value_type, index, bits> f1;
    };
    // set all of the bits to 1.
    vt = std::numeric_limits<value_type>::max ();
    // now blast values into f1.
    f1 = value_type{0};
    using assign_type = typename decltype (f1)::assign_type;
    EXPECT_EQ (f1.value (), static_cast<assign_type> (0U));
    f1 = value_type{1};
    EXPECT_EQ (f1.value (), static_cast<assign_type> (1U));
    constexpr auto v = decltype (f1)::max ();
    EXPECT_EQ (pstore::bit_count::pop_count (v), bits);
    f1 = v;
    // access the value of f1 by casting directly rather than using the value() member function.
    EXPECT_EQ (static_cast<value_type> (f1), v);
    EXPECT_EQ (vt, std::numeric_limits<value_type>::max ());
}

TEST (BitField, IsolationFromOtherBitfields) {
    union {
        std::uint8_t value;
        pstore::bit_field<std::uint8_t, 0, 2> f1; // f1 is bits [0-2)
        pstore::bit_field<std::uint8_t, 2, 6> f2; // f2 is bits [2-8)
    };

    value = 0;
    EXPECT_EQ (f1, 0U);
    EXPECT_EQ (f2, 0U);

    f1 = decltype (f1)::max ();
    EXPECT_EQ (f1, decltype (f1)::max ());
    EXPECT_EQ (f2, 0x00U);

    f1 = std::uint8_t{0};
    f2 = decltype (f2)::max ();
    EXPECT_EQ (f2, decltype (f2)::max ());
    EXPECT_EQ (f1, 0x00U);

    EXPECT_EQ (value, 0xFC);
}

TEST (BitField, Addition) {
    union {
        std::uint8_t v;
        pstore::bit_field<std::uint8_t, 0, 2> f1;
    };
    v = 0U;
    pstore::bit_field<std::uint8_t, 0, 2> const r1 = ++f1;
    EXPECT_EQ (f1, 1U);
    EXPECT_EQ (r1, 1U);
    pstore::bit_field<std::uint8_t, 0, 2> const r2 = f1++;
    EXPECT_EQ (f1, 2U);
    EXPECT_EQ (r2, 1U);
    f1 = std::uint8_t{1};
    f1.operator+= (2U);
    EXPECT_EQ (f1, 3U);
    EXPECT_EQ (v, 3U);
}

TEST (BitField, Subtraction) {
    union {
        std::uint8_t v;
        pstore::bit_field<std::uint8_t, 0, 2> f1;
    };
    v = 0U;
    f1 = std::uint8_t{3};
    pstore::bit_field<std::uint8_t, 0, 2> const r1 = --f1;
    EXPECT_EQ (f1, 2U);
    EXPECT_EQ (r1, 2U);
    pstore::bit_field<std::uint8_t, 0, 2> const r2 = f1--;
    EXPECT_EQ (f1, 1U);
    EXPECT_EQ (r2, 2U);
    f1 = std::uint8_t{3};
    f1.operator-=(std::uint8_t{2});
    EXPECT_EQ (f1, 1U);
    EXPECT_EQ (v, 1U);
}

TEST (BitField, OneBitBoolean) {
    union {
        std::uint8_t v;
        pstore::bit_field<std::uint8_t, 3, 1> f1;
    };
    v = 0U;
    f1 = true;
    EXPECT_TRUE (f1.value ());
    EXPECT_TRUE (static_cast<bool> (f1));
    f1 = false;
    EXPECT_FALSE (f1.value ());
    EXPECT_FALSE (static_cast<bool> (f1));
}

TEST (BitField, Max) {
    EXPECT_EQ ((pstore::bit_field<std::uint8_t, 0, 1>::max ()), 1U);
    EXPECT_EQ ((pstore::bit_field<std::uint16_t, 0, 1>::max ()), 1U);
    EXPECT_EQ ((pstore::bit_field<std::uint32_t, 0, 1>::max ()), 1U);
    EXPECT_EQ ((pstore::bit_field<std::uint64_t, 0, 1>::max ()), 1U);

    EXPECT_EQ ((pstore::bit_field<std::uint8_t, 0, 8>::max ()),
               std::numeric_limits<std::uint8_t>::max ());
    EXPECT_EQ ((pstore::bit_field<std::uint16_t, 0, 8>::max ()),
               std::numeric_limits<std::uint8_t>::max ());
    EXPECT_EQ ((pstore::bit_field<std::uint32_t, 0, 8>::max ()),
               std::numeric_limits<std::uint8_t>::max ());
    EXPECT_EQ ((pstore::bit_field<std::uint64_t, 0, 8>::max ()),
               std::numeric_limits<std::uint8_t>::max ());

    EXPECT_EQ ((pstore::bit_field<std::uint16_t, 0, 16>::max ()),
               std::numeric_limits<std::uint16_t>::max ());
    EXPECT_EQ ((pstore::bit_field<std::uint32_t, 0, 16>::max ()),
               std::numeric_limits<std::uint16_t>::max ());
    EXPECT_EQ ((pstore::bit_field<std::uint64_t, 0, 16>::max ()),
               std::numeric_limits<std::uint16_t>::max ());

    EXPECT_EQ ((pstore::bit_field<std::uint32_t, 0, 32>::max ()),
               std::numeric_limits<std::uint32_t>::max ());
    EXPECT_EQ ((pstore::bit_field<std::uint64_t, 0, 32>::max ()),
               std::numeric_limits<std::uint32_t>::max ());

    EXPECT_EQ ((pstore::bit_field<std::uint64_t, 0, 64>::max ()),
               std::numeric_limits<std::uint64_t>::max ());
}
