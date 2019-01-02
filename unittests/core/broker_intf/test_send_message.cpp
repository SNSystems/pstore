//*                     _                                             *
//*  ___  ___ _ __   __| |  _ __ ___   ___  ___ ___  __ _  __ _  ___  *
//* / __|/ _ \ '_ \ / _` | | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ *
//* \__ \  __/ | | | (_| | | | | | | |  __/\__ \__ \ (_| | (_| |  __/ *
//* |___/\___|_| |_|\__,_| |_| |_| |_|\___||___/___/\__,_|\__, |\___| *
//*                                                       |___/       *
//===- unittests/core/broker_intf/test_send_message.cpp -------------------===//
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
/// \file test_send_message.cpp

#include "pstore/broker_intf/send_message.hpp"

#include "pstore/support/portab.hpp"
#include "gmock/gmock.h"

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/broker_intf/message_type.hpp"
#include "pstore/broker_intf/writer.hpp"

namespace {

    auto make_pipe () -> pstore::broker::fifo_path::client_pipe { return pstore::broker::fifo_path::client_pipe{}; }

    class mock_writer : public pstore::broker::writer {
    public:
        mock_writer ()
                : pstore::broker::writer (make_pipe ()) {}
        MOCK_METHOD1 (write_impl, bool(pstore::broker::message_type const &));
    };

    class BrokerSendMessage : public ::testing::Test {
    public:
        BrokerSendMessage ()
                : message_id_{pstore::broker::next_message_id ()} {}

    protected:
        std::uint32_t const message_id_;
    };

} // namespace


TEST_F (BrokerSendMessage, SinglePart) {
    using ::testing::Eq;
    using ::testing::Return;

    mock_writer wr;
    pstore::broker::message_type const expected{message_id_, 0, 1, "hello world"};
    EXPECT_CALL (wr, write_impl (Eq (expected))).WillOnce (Return (true));

    pstore::broker::send_message (wr, true /*error on timeout*/, "hello", "world");
}

TEST_F (BrokerSendMessage, TwoParts) {
    using ::testing::Eq;
    using ::testing::Return;

    std::string const verb = "verb";
    auto const part1_chars = pstore::broker::message_type::payload_chars - verb.length () - 1U;

    // Increase the length by 1 to cause the payload to overflow into a second message.
    std::string::size_type const payload_length = part1_chars + 1U;
    std::string const path (payload_length, 'p');

    auto const part2_chars = payload_length - part1_chars;

    mock_writer wr;

    pstore::broker::message_type const expected1 (message_id_, 0, 2,
                                                  verb + ' ' + std::string (part1_chars, 'p'));
    pstore::broker::message_type const expected2 (message_id_, 1, 2,
                                                  std::string (part2_chars, 'p'));
    EXPECT_CALL (wr, write_impl (Eq (expected1))).WillOnce (Return (true));
    EXPECT_CALL (wr, write_impl (Eq (expected2))).WillOnce (Return (true));

    pstore::broker::send_message (wr, true /*error on timeout*/, verb.c_str (), path.c_str ());
}
