//*   __ _ _        _                     _ _       *
//*  / _(_) | ___  | |__   __ _ _ __   __| | | ___  *
//* | |_| | |/ _ \ | '_ \ / _` | '_ \ / _` | |/ _ \ *
//* |  _| | |  __/ | | | | (_| | | | | (_| | |  __/ *
//* |_| |_|_|\___| |_| |_|\__,_|_| |_|\__,_|_|\___| *
//*                                                 *
//===- unittests/os/test_file_handle.cpp ----------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
#include "pstore/os/file.hpp"

#include <array>
#include <memory>

// 3rd party includes
#include "gmock/gmock.h"
#include "gtest/gtest.h"

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
    TypeParam * ptr = nullptr;
    constexpr std::size_t size = std::numeric_limits<std::uint8_t>::max ();
    using ::testing::Return;
    EXPECT_CALL (this->callback, call (ptr, size)).Times (1).WillOnce (Return (size));

    using pstore::file::details::split;
    std::size_t const total = split<std::uint8_t> (ptr, size, this->callback);
    EXPECT_EQ (size, total);
}

TYPED_TEST (SplitFixture, Uint16Max) {
    TypeParam * ptr = nullptr;
    constexpr std::size_t size = std::numeric_limits<std::uint16_t>::max ();
    using ::testing::Return;
    EXPECT_CALL (this->callback, call (ptr, size)).Times (1).WillOnce (Return (size));

    using pstore::file::details::split;
    std::size_t const total = split<std::uint16_t> (ptr, size, this->callback);
    EXPECT_EQ (size, total);
}

TYPED_TEST (SplitFixture, SplitUint16MaxPlus1) {
    struct params {
        TypeParam * ptr;
        std::size_t size;
    };
    params const call1{nullptr, std::numeric_limits<std::uint16_t>::max ()};
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
    params const call1{nullptr, std::numeric_limits<std::uint8_t>::max ()};
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

