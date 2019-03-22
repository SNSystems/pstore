//*                                  *
//*  _ __   __ _ _ __ ___  ___ _ __  *
//* | '_ \ / _` | '__/ __|/ _ \ '__| *
//* | |_) | (_| | |  \__ \  __/ |    *
//* | .__/ \__,_|_|  |___/\___|_|    *
//* |_|                              *
//===- unittests/broker/test_parser.cpp -----------------------------------===//
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
#include "pstore/broker/parser.hpp"

#include <gmock/gmock.h>

#include "pstore/broker_intf/message_type.hpp"

TEST (MessageParse, SinglePartCommand) {
    using namespace pstore::broker;
    pstore::broker::partial_cmds cmds;
    std::unique_ptr<broker_command> command =
        parse (message_type (1234, 0, 1, std::string{"HELO hello world"}), cmds);
    ASSERT_NE (command.get (), nullptr);
    EXPECT_EQ (command->verb, "HELO");
    EXPECT_EQ (command->path, "hello world");
    EXPECT_EQ (cmds.size (), 0U);
}

TEST (MessageParse, TwoPartCommandInOrder) {
    using namespace pstore::broker;
    static constexpr std::uint32_t message_id = 1234;

    pstore::broker::partial_cmds cmds;
    std::unique_ptr<broker_command> c1 =
        parse (message_type (message_id, 0, 2, std::string{"HELO to be"}), cmds);
    EXPECT_EQ (cmds.size (), 1U);
    EXPECT_EQ (c1.get (), nullptr);
    std::unique_ptr<broker_command> c2 =
        parse (message_type (message_id, 1, 2, std::string{" or not to be"}), cmds);
    ASSERT_NE (c2.get (), nullptr);
    EXPECT_EQ (c2->verb, "HELO");
    EXPECT_EQ (c2->path, "to be or not to be");
    EXPECT_TRUE (cmds.empty ());
}

TEST (MessageParse, TwoPartCommandOutOfOrder) {
    using namespace pstore::broker;
    static constexpr std::uint32_t message_id = 1234;

    pstore::broker::partial_cmds cmds;
    std::unique_ptr<broker_command> c1 =
        parse (message_type (message_id, 1, 2, std::string{" or not to be"}), cmds);
    EXPECT_EQ (cmds.size (), 1U);
    EXPECT_EQ (c1.get (), nullptr);
    std::unique_ptr<broker_command> c2 =
        parse (message_type (message_id, 0, 2, std::string{"HELO to be"}), cmds);
    ASSERT_NE (c2.get (), nullptr);
    EXPECT_EQ (c2->verb, "HELO");
    EXPECT_EQ (c2->path, "to be or not to be");
    EXPECT_TRUE (cmds.empty ());
}
