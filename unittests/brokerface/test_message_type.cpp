//===- unittests/brokerface/test_message_type.cpp -------------------------===//
//*                                             _                     *
//*  _ __ ___   ___  ___ ___  __ _  __ _  ___  | |_ _   _ _ __   ___  *
//* | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ | __| | | | '_ \ / _ \ *
//* | | | | | |  __/\__ \__ \ (_| | (_| |  __/ | |_| |_| | |_) |  __/ *
//* |_| |_| |_|\___||___/___/\__,_|\__, |\___|  \__|\__, | .__/ \___| *
//*                                |___/            |___/|_|          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/// \file test_message_type.cpp

#include "pstore/brokerface/message_type.hpp"

// Standard library includes
#include <list>

// 3rd party includes
#include <gmock/gmock.h>

// pstore includes
#include "pstore/support/error.hpp"

// Local includes
#include "check_for_error.hpp"

TEST (BrokerMessageType, BadPartNo) {
    auto create = [] () {
        std::uint32_t const mid = 3;
        std::uint16_t const part = 2;
        std::uint16_t const num_parts = 2;
        return pstore::brokerface::message_type (mid, part, num_parts, "");
    };
    check_for_error (create, ::pstore::error_code::bad_message_part_number);
}

TEST (BrokerMessageType, EmptyString) {
    std::uint32_t const mid = 1234;
    std::uint16_t const part = 21;
    std::uint16_t const num_parts = 1234;

    pstore::brokerface::message_type const actual{mid, part, num_parts, ""};

    EXPECT_EQ (actual.sender_id, pstore::brokerface::message_type::process_id);
    EXPECT_EQ (actual.message_id, mid);
    EXPECT_EQ (actual.part_no, part);
    EXPECT_EQ (actual.num_parts, num_parts);

    pstore::brokerface::message_type::payload_type expected_payload{{'\0'}};
    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}

TEST (BrokerMessageType, ShortString) {
    std::string const payload = "hello world";
    pstore::brokerface::message_type const actual{0, 0, 1, payload};

    pstore::brokerface::message_type::payload_type expected_payload{{'\0'}};
    std::copy (std::begin (payload), std::end (payload), std::begin (expected_payload));

    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}

TEST (BrokerMessageType, LongStringIsTruncated) {
    std::string const long_payload (pstore::brokerface::message_type::payload_chars + 1, 'A');

    pstore::brokerface::message_type const actual{0, 0, 1, long_payload};

    pstore::brokerface::message_type::payload_type expected_payload{{'\0'}};
    std::fill (std::begin (expected_payload), std::end (expected_payload), 'A');

    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}

TEST (BrokerMessageType, ShortPayloadUsingIterator) {
    std::string const payload = "hello world";
    pstore::brokerface::message_type const actual{0, 0, 1, std::begin (payload),
                                                  std::end (payload)};

    pstore::brokerface::message_type::payload_type expected_payload{{'\0'}};
    std::copy (std::begin (payload), std::end (payload), std::begin (expected_payload));

    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}

namespace {

    template <typename OutputIterator>
    void generate (OutputIterator out, std::size_t num) {
        auto ctr = 0U;
        std::generate_n (out, num, [&ctr] () { return static_cast<char> (ctr++ % 26 + 'A'); });
    }

} // end anonymous namespace

TEST (BrokerMessageType, MaxLengthIteratorRange) {
    std::string long_payload;
    constexpr auto num = std::size_t{pstore::brokerface::message_type::payload_chars};
    generate (std::back_inserter (long_payload), num);

    pstore::brokerface::message_type const actual{0, 0, 1, std::begin (long_payload),
                                                  std::end (long_payload)};

    pstore::brokerface::message_type::payload_type expected_payload{{'\0'}};
    generate (std::begin (expected_payload), expected_payload.size ());
    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}

TEST (BrokerMessageType, TooLongIteratorRangeIsTruncated) {
    std::list<char>
        long_payload; // deliberately using list<> because it's quite different from array<>/string.
    constexpr auto num = std::size_t{pstore::brokerface::message_type::payload_chars + 1};
    generate (std::back_inserter (long_payload), num);

    pstore::brokerface::message_type const actual (0, 0, 1, std::begin (long_payload),
                                                   std::end (long_payload));

    pstore::brokerface::message_type::payload_type expected_payload{{'\0'}};
    generate (std::begin (expected_payload), expected_payload.size ());
    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}

TEST (BrokerMessageType, NegativeDistanceBetweenIterators) {
    char const payload[] = "payload";
    // Note that first and last are reversed.
    auto last = std::begin (payload);
    auto first = std::end (payload);
    ASSERT_LT (std::distance (first, last), 0);

    pstore::brokerface::message_type const actual (0, 0, 1, first, last);

    pstore::brokerface::message_type::payload_type expected_payload{{'\0'}};
    EXPECT_THAT (actual.payload, ::testing::ContainerEq (expected_payload));
}
