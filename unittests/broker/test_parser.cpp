//===- unittests/broker/test_parser.cpp -----------------------------------===//
//*                                  *
//*  _ __   __ _ _ __ ___  ___ _ __  *
//* | '_ \ / _` | '__/ __|/ _ \ '__| *
//* | |_) | (_| | |  \__ \  __/ |    *
//* | .__/ \__,_|_|  |___/\___|_|    *
//* |_|                              *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/broker/parser.hpp"

#include <gmock/gmock.h>

#include "pstore/brokerface/message_type.hpp"

using namespace std::string_literals;

TEST (MessageParse, SinglePartCommand) {
    using namespace pstore::broker;
    pstore::broker::partial_cmds cmds;
    std::unique_ptr<broker_command> command =
        parse (pstore::brokerface::message_type (1234, 0, 1, "HELO hello world"s), cmds);
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
        parse (pstore::brokerface::message_type (message_id, 0, 2, "HELO to be"s), cmds);
    EXPECT_EQ (cmds.size (), 1U);
    EXPECT_EQ (c1.get (), nullptr);
    std::unique_ptr<broker_command> c2 =
        parse (pstore::brokerface::message_type (message_id, 1, 2, " or not to be"s), cmds);
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
        parse (pstore::brokerface::message_type (message_id, 1, 2, " or not to be"s), cmds);
    EXPECT_EQ (cmds.size (), 1U);
    EXPECT_EQ (c1.get (), nullptr);
    std::unique_ptr<broker_command> c2 =
        parse (pstore::brokerface::message_type (message_id, 0, 2, "HELO to be"s), cmds);
    ASSERT_NE (c2.get (), nullptr);
    EXPECT_EQ (c2->verb, "HELO");
    EXPECT_EQ (c2->path, "to be or not to be");
    EXPECT_TRUE (cmds.empty ());
}
