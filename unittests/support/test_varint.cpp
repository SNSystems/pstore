//*                  _       _    *
//* __   ____ _ _ __(_)_ __ | |_  *
//* \ \ / / _` | '__| | '_ \| __| *
//*  \ V / (_| | |  | | | | | |_  *
//*   \_/ \__,_|_|  |_|_| |_|\__| *
//*                               *
//===- unittests/support/test_varint.cpp ----------------------------------===//
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
#include "pstore/support/varint.hpp"
#include <cstdint>
#include <vector>
#include "gmock/gmock.h"
#include "binary.hpp"

namespace {

    class VarInt : public ::testing::Test {
    protected:
        static std::uint64_t all_ones (unsigned places) {
            assert (places < 64);
            return (UINT64_C (1) << places) - 1;
        }

        static std::uint64_t power (unsigned exponent) {
            assert (exponent < 64);
            return UINT64_C (1) << exponent;
        }

        static void check (std::uint64_t test_value,
                           std::initializer_list<std::uint8_t> const & il) {
            EXPECT_EQ (il.size (), pstore::varint::encoded_size (test_value));
            std::vector<std::uint8_t> buffer;
            pstore::varint::encode (test_value, std::back_inserter (buffer));
            EXPECT_THAT (buffer, ::testing::ElementsAreArray (il));
            EXPECT_EQ (buffer.size (), pstore::varint::decode_size (buffer.data ()));
            EXPECT_EQ (test_value, pstore::varint::decode (buffer.data ()));
        }
    };

} // namespace

//         +---------------------------+-----+
// bit     | 7   6   5   4   3   2   1 |  0  |
//         +---------------------------+-----+
// meaning |           value           | (*) |
//         +---------------------------+-----+
// value   | 0 | 0 | 0 | 0 | 0 | 0 | 0 |  1  |
//         +---------------------------+-----+
// (*) "1 byte"
TEST_F (VarInt, Zero) {
    check (UINT64_C (0), {binary<std::uint8_t, 0, 0, 0, 0, 0, 0, 0, 1>::value});
}

//         +---------------------------+-----+
// bit     | 7   6   5   4   3   2   1 |  0  |
//         +---------------------------+-----+
// meaning |           value           | (*) |
//         +---------------------------+-----+
// value   | 0 | 0 | 0 | 0 | 0 | 0 | 1 |  1  |
//         +---------------------------+-----+
// (*) "1 byte"
TEST_F (VarInt, One) {
    check (UINT64_C (1), {binary<std::uint8_t, 0, 0, 0, 0, 0, 0, 1, 1>::value});
}

//         +---------------------------+-----+
// bit     | 7   6   5   4   3   2   1 |  0  |
//         +---------------------------+-----+
// meaning |           value           | (*) |
//         +---------------------------+-----+
// value   | 1 | 1 | 1 | 1 | 1 | 1 | 1 |  1  |
//         +---------------------------+-----+
// (*) "1 byte"
TEST_F (VarInt, 7Bits) {
    check (all_ones (7), {0xFF});
}

//                      byte 0                            byte 1
//         +-----------------------+-------+ +-------------------------------+
// bit     | 7   6   5   4   3   2 | 1   0 | | 7   6   5   4   3   2   1   0 |
//         +-----------------------+-------+ +-------------------------------+
// meaning |         value         |   2   | |             value             |
//         |       bits 0-5        | bytes | |           bits 6-13           |
//         +-----------------------+-------+ +--------------------------------
// value   | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 |
//         +-----------------------+-------+ +-------------------------------+
TEST_F (VarInt, 2pow8) {
    check (power (8), {binary<std::uint8_t, 0, 0, 0, 0, 0, 0, 1, 0>::value,
                       binary<std::uint8_t, 0, 0, 0, 0, 0, 1, 0, 0>::value});
}

//                      byte 0                            byte 1
//         +-----------------------+-------+ +-------------------------------+
// bit     | 7   6   5   4   3   2 | 1   0 | | 7   6   5   4   3   2   1   0 |
//         +-----------------------+-------+ +-------------------------------+
// meaning |         value         |   2   | |             value             |
//         |       bits 0-5        | bytes | |           bits 6-13           |
//         +-----------------------+-------+ +--------------------------------
// value   | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 0 | | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
//         +-----------------------+-------+ +-------------------------------+
TEST_F (VarInt, 14Bits) {
    check (all_ones (14), {binary<std::uint8_t, 1, 1, 1, 1, 1, 1, 1, 0>::value, 0xFF});
}

//                      byte 0                byte1             byte 2
//         +-------------------+-----------+         +-------------------------------+
// bit     | 7   6   5   4   3 | 2   1   0 |         | 7   6   5   4   3   2   1   0 |
//         +-------------------+-----------+         +-------------------------------+
// meaning |         value     |     3     |   ...   |             value             |
//         |       bits 0-4    |   bytes   |         |           bits 13-20          |
//         +-------------------+-----------+         +--------------------------------
// value   | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 |         | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 |
//         +-------------------+-----------+         +-------------------------------+
TEST_F (VarInt, 2pow14) {
    check (power (14), {0x04, 0, 0x02});
}

//                      byte 0                byte1             byte 2
//         +-------------------+-----------+         +-------------------------------+
// bit     | 7   6   5   4   3 | 2   1   0 |         | 7   6   5   4   3   2   1   0 |
//         +-------------------+-----------+         +-------------------------------+
// meaning |         value     |     3     |   ...   |             value             |
//         |       bits 0-4    |   bytes   |         |           bits 13-20          |
//         +-------------------+-----------+         +--------------------------------
// value   | 1 | 1 | 1 | 1 | 1 | 1 | 0 | 0 |         | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
//         +-------------------+-----------+         +-------------------------------+
TEST_F (VarInt, 21Bits) {
    check (all_ones (21), {0xFC, 0xFF, 0xFF});
}
TEST_F (VarInt, 2pow21) {
    check (power (21), {binary<std::uint8_t, 0, 0, 0, 0, 1, 0, 0, 0>::value, 0, 0,
                        binary<std::uint8_t, 0, 0, 0, 0, 0, 0, 1, 0>::value});
}
TEST_F (VarInt, 28Bits) {
    check (all_ones (28), {binary<std::uint8_t, 1, 1, 1, 1, 1, 0, 0, 0>::value, 0xFF, 0xFF, 0xFF});
}
TEST_F (VarInt, 2pow28) {
    check (power (28), {binary<std::uint8_t, 0, 0, 0, 1, 0, 0, 0, 0>::value, 0, 0, 0,
                        binary<std::uint8_t, 0, 0, 0, 0, 0, 0, 1, 0>::value});
}
TEST_F (VarInt, 35Bits) {
    check (all_ones (35),
           {binary<std::uint8_t, 1, 1, 1, 1, 0, 0, 0, 0>::value, 0xFF, 0xFF, 0xFF, 0xFF});
}
TEST_F (VarInt, 2pow35) {
    check (power (35), {binary<std::uint8_t, 0, 0, 1, 0, 0, 0, 0, 0>::value, 0, 0, 0, 0,
                        binary<std::uint8_t, 0, 0, 0, 0, 0, 0, 1, 0>::value});
}
TEST_F (VarInt, 42Bits) {
    check (all_ones (42),
           {binary<std::uint8_t, 1, 1, 1, 0, 0, 0, 0, 0>::value, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
}
TEST_F (VarInt, 2pow42) {
    check (power (42), {binary<std::uint8_t, 0, 1, 0, 0, 0, 0, 0, 0>::value, 0, 0, 0, 0, 0,
                        binary<std::uint8_t, 0, 0, 0, 0, 0, 0, 1, 0>::value});
}
TEST_F (VarInt, 49Bits) {
    check (all_ones (49), {binary<std::uint8_t, 1, 1, 0, 0, 0, 0, 0, 0>::value, 0xFF, 0xFF, 0xFF,
                           0xFF, 0xFF, 0xFF});
}
TEST_F (VarInt, 2pow49) {
    check (power (49), {binary<std::uint8_t, 1, 0, 0, 0, 0, 0, 0, 0>::value, 0, 0, 0, 0, 0, 0,
                        binary<std::uint8_t, 0, 0, 0, 0, 0, 0, 1, 0>::value});
}
TEST_F (VarInt, 56Bits) {
    check (all_ones (56), {binary<std::uint8_t, 1, 0, 0, 0, 0, 0, 0, 0>::value, 0xFF, 0xFF, 0xFF,
                           0xFF, 0xFF, 0xFF, 0xFF});
}
TEST_F (VarInt, 2pow63) {
    check (power (63),
           {0, 0, 0, 0, 0, 0, 0, 0, binary<std::uint8_t, 1, 0, 0, 0, 0, 0, 0, 0>::value});
}
TEST_F (VarInt, 64Bits) {
    check (~UINT64_C (0), {0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
}
