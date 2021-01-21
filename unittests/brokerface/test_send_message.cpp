//*                     _                                             *
//*  ___  ___ _ __   __| |  _ __ ___   ___  ___ ___  __ _  __ _  ___  *
//* / __|/ _ \ '_ \ / _` | | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ *
//* \__ \  __/ | | | (_| | | | | | | |  __/\__ \__ \ (_| | (_| |  __/ *
//* |___/\___|_| |_|\__,_| |_| |_| |_|\___||___/___/\__,_|\__, |\___| *
//*                                                       |___/       *
//===- unittests/brokerface/test_send_message.cpp -------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file test_send_message.cpp

#include "pstore/brokerface/send_message.hpp"

#include <gmock/gmock.h>

#include "pstore/brokerface/message_type.hpp"
#include "pstore/brokerface/writer.hpp"
#include "pstore/os/descriptor.hpp"

namespace {

    auto make_pipe () -> pstore::brokerface::fifo_path::client_pipe {
        return pstore::brokerface::fifo_path::client_pipe{};
    }

    class mock_writer : public pstore::brokerface::writer {
    public:
        mock_writer ()
                : pstore::brokerface::writer (make_pipe ()) {}
        MOCK_METHOD1 (write_impl, bool (pstore::brokerface::message_type const &));
    };

    class BrokerSendMessage : public ::testing::Test {
    public:
        BrokerSendMessage ()
                : message_id_{pstore::brokerface::next_message_id ()} {}

    protected:
        std::uint32_t const message_id_;
    };

} // end anonymous namespace


TEST_F (BrokerSendMessage, SinglePart) {
    using ::testing::Eq;
    using ::testing::Return;

    mock_writer wr;
    pstore::brokerface::message_type const expected{message_id_, 0, 1, "hello world"};
    EXPECT_CALL (wr, write_impl (Eq (expected))).WillOnce (Return (true));

    pstore::brokerface::send_message (wr, true /*error on timeout*/, "hello", "world");
}

TEST_F (BrokerSendMessage, TwoParts) {
    using ::testing::Eq;
    using ::testing::Return;

    std::string const verb = "verb";
    auto const part1_chars = pstore::brokerface::message_type::payload_chars - verb.length () - 1U;

    // Increase the length by 1 to cause the payload to overflow into a second message.
    std::string::size_type const payload_length = part1_chars + 1U;
    std::string const path (payload_length, 'p');

    auto const part2_chars = payload_length - part1_chars;

    mock_writer wr;

    pstore::brokerface::message_type const expected1 (message_id_, 0, 2,
                                                      verb + ' ' + std::string (part1_chars, 'p'));
    pstore::brokerface::message_type const expected2 (message_id_, 1, 2,
                                                      std::string (part2_chars, 'p'));
    EXPECT_CALL (wr, write_impl (Eq (expected1))).WillOnce (Return (true));
    EXPECT_CALL (wr, write_impl (Eq (expected2))).WillOnce (Return (true));

    pstore::brokerface::send_message (wr, true /*error on timeout*/, verb.c_str (), path.c_str ());
}
