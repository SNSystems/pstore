//*                                             _                     *
//*  _ __ ___   ___  ___ ___  __ _  __ _  ___  | |_ _   _ _ __   ___  *
//* | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ | __| | | | '_ \ / _ \ *
//* | | | | | |  __/\__ \__ \ (_| | (_| |  __/ | |_| |_| | |_) |  __/ *
//* |_| |_| |_|\___||___/___/\__,_|\__, |\___|  \__|\__, | .__/ \___| *
//*                                |___/            |___/|_|          *
//===- unittests/pstore/broker_intf/test_message_type.cpp -----------------===//
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

/// \file test_message_type.cpp

#include "pstore_broker_intf/message_type.hpp"
#include <list>
#include "gmock/gmock.h"

#include "pstore_support/error.hpp"
#include "check_for_error.hpp"

TEST (BrokerMessageType, BadPartNo) {
    auto create = []() {
        std::uint32_t const mid = 3;
        std::uint16_t const part = 2;
        std::uint16_t const num_parts = 2;
        return pstore::broker::message_type (mid, part, num_parts, "");
    };
    check_for_error (create, ::pstore::error_code::bad_message_part_number);
}


TEST (BrokerMessageType, EmptyString) {
    std::uint32_t const mid = 1234;
    std::uint16_t const part = 21;
    std::uint16_t const num_parts = 1234;

    pstore::broker::message_type const actual {mid, part, num_parts, ""};

    EXPECT_EQ (actual.sender_id, pstore::broker::message_type::process_id);
    EXPECT_EQ (actual.message_id, mid);
    EXPECT_EQ (actual.part_no, part);
    EXPECT_EQ (actual.num_parts, num_parts);

    pstore::broker::message_type::payload_type expected_payload{{'\0'}};
    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}

TEST (BrokerMessageType, ShortString) {
    std::string const payload = "hello world";
    pstore::broker::message_type const actual {0, 0, 1, payload};

    pstore::broker::message_type::payload_type expected_payload{{'\0'}};
    std::copy (std::begin (payload), std::end (payload), std::begin (expected_payload));

    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}

TEST (BrokerMessageType, LongStringIsTruncated) {
    std::string const long_payload (pstore::broker::message_type::payload_chars + 1, 'A');

    pstore::broker::message_type const actual {0, 0, 1, long_payload};

    pstore::broker::message_type::payload_type expected_payload;
    std::fill (std::begin (expected_payload), std::end (expected_payload), 'A');

    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}



TEST (BrokerMessageType, ShortPayloadUsingIterator) {
    std::string const payload = "hello world";
    pstore::broker::message_type const actual {0, 0, 1, std::begin (payload), std::end (payload)};

    pstore::broker::message_type::payload_type expected_payload{{'\0'}};
    std::copy (std::begin (payload), std::end (payload), std::begin (expected_payload));

    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}

namespace {

    template <typename OutputIterator>
    void generate (OutputIterator out, std::size_t num) {
        auto ctr = 0U;
        std::generate_n (out, num, [&ctr] () -> char { return ctr++ % 26 + 'A'; });
    }

}

TEST (BrokerMessageType, MaxLengthIteratorRange) {
    std::string long_payload;
    constexpr auto num = std::size_t {pstore::broker::message_type::payload_chars};
    generate (std::back_inserter (long_payload), num);

    pstore::broker::message_type const actual {0, 0, 1, std::begin (long_payload), std::end (long_payload)};

    pstore::broker::message_type::payload_type expected_payload;
    generate (std::begin (expected_payload), expected_payload.size ());
    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}

TEST (BrokerMessageType, TooLongIteratorRangeIsTruncated) {
    std::list<char> long_payload; // deliberately using list<> because it's quite different from array<>/string.
    constexpr auto num = std::size_t{pstore::broker::message_type::payload_chars + 1};
    generate (std::back_inserter (long_payload), num);

    pstore::broker::message_type const actual (0, 0, 1, std::begin (long_payload), std::end (long_payload));

    pstore::broker::message_type::payload_type expected_payload;
    generate (std::begin (expected_payload), expected_payload.size ());
    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}



TEST (BrokerMessageType, NegativeDistanceBetweenIterators) {
    char const payload [] = "payload";
    auto last = payload;
    auto first = payload + sizeof (payload) / sizeof (payload [0]);

    pstore::broker::message_type const actual (0, 0, 1, first, last);

    pstore::broker::message_type::payload_type expected_payload{{'\0'}};
    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}
// eof: unittests/pstore/broker_intf/test_message_type.cpp
