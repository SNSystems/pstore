//===- unittests/os/test_file_handle.cpp ----------------------------------===//
//*   __ _ _        _                     _ _       *
//*  / _(_) | ___  | |__   __ _ _ __   __| | | ___  *
//* | |_| | |/ _ \ | '_ \ / _` | '_ \ / _` | |/ _ \ *
//* |  _| | |  __/ | | | | (_| | | | | (_| | |  __/ *
//* |_| |_|_|\___| |_| |_|\__,_|_| |_|\__,_|_|\___| *
//*                                                 *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/os/file.hpp"

// Standard library includes
#include <array>
#include <memory>

// 3rd party includes
#include <gmock/gmock.h>

namespace {

    template <typename BufferType>
    class SplitFixture : public ::testing::Test {
    protected:
        struct mock_callback {
            MOCK_CONST_METHOD2 (call, std::size_t (void const * p, std::size_t size));
            std::size_t operator() (void const * p, std::size_t size) const {
                return call (p, size);
            }
        };
        mock_callback callback;
    };

    using split_types = ::testing::Types<std::uint8_t const, std::uint8_t>;

} // end anonymous namespace

#ifdef PSTORE_IS_INSIDE_LLVM
TYPED_TEST_CASE (SplitFixture, split_types);
#else
TYPED_TEST_SUITE (SplitFixture, split_types, );
#endif // PSTORE_IS_INSIDE_LLVM

TYPED_TEST (SplitFixture, Empty) {
    TypeParam buffer{};
    constexpr std::size_t size{0};
    EXPECT_CALL (this->callback, call (&buffer, size)).Times (0);

    using pstore::file::details::split;
    std::size_t const total = split<std::uint16_t> (&buffer, size, this->callback);
    EXPECT_EQ (size, total);
}

TYPED_TEST (SplitFixture, Small) {
    constexpr std::size_t size{10};
    TypeParam buffer[size] = {};

    using ::testing::Return;
    EXPECT_CALL (this->callback, call (buffer, size)).Times (1).WillOnce (Return (size));

    using pstore::file::details::split;
    std::size_t const total = split<std::uint16_t> (buffer, sizeof (buffer), this->callback);
    EXPECT_EQ (size, total);
}

TYPED_TEST (SplitFixture, Uint8Max) {
    constexpr std::size_t size = std::numeric_limits<std::uint8_t>::max ();
    std::array<TypeParam, size> buffer{size, TypeParam{}};

    using ::testing::Return;
    EXPECT_CALL (this->callback, call (buffer.data (), size)).Times (1).WillOnce (Return (size));

    using pstore::file::details::split;
    std::size_t const total = split<std::uint8_t> (buffer.data (), size, this->callback);
    EXPECT_EQ (size, total);
}

TYPED_TEST (SplitFixture, Uint16Max) {
    constexpr std::size_t size = std::numeric_limits<std::uint16_t>::max ();
    auto buffer = std::make_unique<TypeParam[]> (size);

    using ::testing::Return;
    EXPECT_CALL (this->callback, call (buffer.get (), size)).Times (1).WillOnce (Return (size));

    using pstore::file::details::split;
    std::size_t const total = split<std::uint16_t> (buffer.get (), size, this->callback);
    EXPECT_EQ (size, total);
}

TYPED_TEST (SplitFixture, SplitUint16MaxPlus1) {
    struct params {
        TypeParam * ptr;
        std::size_t size;
    };
    auto buffer = std::make_unique<TypeParam[]> (std::numeric_limits<std::uint16_t>::max () + 1U);

    params const call1{buffer.get (), std::numeric_limits<std::uint16_t>::max ()};
    params const call2{call1.ptr + call1.size, 1U};
    auto const total_size = call1.size + call2.size;

    // Set up expectations for the callback.
    using ::testing::Return;
    EXPECT_CALL (this->callback, call (call1.ptr, call1.size)).WillOnce (Return (call1.size));
    EXPECT_CALL (this->callback, call (call2.ptr, call2.size)).WillOnce (Return (call2.size));

    // Perform the split() call.
    using ::pstore::file::details::split;
    std::size_t const total = split<std::uint16_t> (call1.ptr, total_size, this->callback);

    // Check the return value.
    EXPECT_EQ (total_size, total);
}

TYPED_TEST (SplitFixture, SplitUint8TwiceMaxPlus1) {
    struct params {
        TypeParam * ptr;
        std::size_t size;
    };
    auto buffer =
        std::make_unique<TypeParam[]> (std::numeric_limits<std::uint8_t>::max () * 2U + 1U);

    params const call1{buffer.get (), std::numeric_limits<std::uint8_t>::max ()};
    params const call2{call1.ptr + call1.size, std::numeric_limits<std::uint8_t>::max ()};
    params const call3{call2.ptr + call2.size, 1U};
    auto const total_size = call1.size + call2.size + call3.size;

    // Set up expectations for the callback.
    using ::testing::Return;
    EXPECT_CALL (this->callback, call (call1.ptr, call1.size)).WillOnce (Return (call1.size));
    EXPECT_CALL (this->callback, call (call2.ptr, call2.size)).WillOnce (Return (call2.size));
    EXPECT_CALL (this->callback, call (call3.ptr, call3.size)).WillOnce (Return (call3.size));

    // Perform the split() call.
    using ::pstore::file::details::split;
    std::size_t const total = split<std::uint8_t> (call1.ptr, total_size, this->callback);

    // Check the return value.
    EXPECT_EQ (total_size, total);
}
