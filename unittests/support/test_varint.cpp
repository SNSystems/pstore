//*                  _       _    *
//* __   ____ _ _ __(_)_ __ | |_  *
//* \ \ / / _` | '__| | '_ \| __| *
//*  \ V / (_| | |  | | | | | |_  *
//*   \_/ \__,_|_|  |_|_| |_|\__| *
//*                               *
//===- unittests/support/test_varint.cpp ----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/support/varint.hpp"

#include <cstdint>
#include <vector>

#include <gmock/gmock.h>

namespace {

    class VarInt : public ::testing::Test {
    protected:
        static std::uint64_t all_ones (unsigned places) {
            PSTORE_ASSERT (places < 64);
            return (UINT64_C (1) << places) - 1;
        }

        static std::uint64_t power (unsigned exponent) {
            PSTORE_ASSERT (exponent < 64);
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
    check (UINT64_C (0), {0b00000001});
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
    check (UINT64_C (1), {0b00000011});
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
    check (power (8), {0b00000010, 0b00000100});
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
    check (all_ones (14), {0b11111110, 0b11111111});
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
    check (power (14), {0b00000100, 0, 0b00000010});
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
    check (power (21), {0b00001000, 0, 0, 0b00000010});
}
TEST_F (VarInt, 28Bits) {
    check (all_ones (28), {0b11111000, 0b11111111, 0b11111111, 0b11111111});
}
TEST_F (VarInt, 2pow28) {
    check (power (28), {0b00010000, 0, 0, 0, 0b00000010});
}
TEST_F (VarInt, 35Bits) {
    check (all_ones (35), {0b11110000, 0b11111111, 0b11111111, 0b11111111, 0b11111111});
}
TEST_F (VarInt, 2pow35) {
    check (power (35), {0b00100000, 0, 0, 0, 0, 0b00000010});
}
TEST_F (VarInt, 42Bits) {
    check (all_ones (42), {0b11100000, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111});
}
TEST_F (VarInt, 2pow42) {
    check (power (42), {0b01000000, 0, 0, 0, 0, 0, 0b00000010});
}
TEST_F (VarInt, 49Bits) {
    check (all_ones (49),
           {0b11000000, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111});
}
TEST_F (VarInt, 2pow49) {
    check (power (49), {0b10000000, 0, 0, 0, 0, 0, 0, 0b00000010});
}
TEST_F (VarInt, 56Bits) {
    check (all_ones (56), {0b10000000, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111,
                           0b11111111, 0b11111111});
}
TEST_F (VarInt, 2pow63) {
    check (power (63), {0, 0, 0, 0, 0, 0, 0, 0, 0b10000000});
}
TEST_F (VarInt, 64Bits) {
    check (~UINT64_C (0), {0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
}
