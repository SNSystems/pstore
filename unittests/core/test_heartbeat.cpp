//*  _                     _   _                _    *
//* | |__   ___  __ _ _ __| |_| |__   ___  __ _| |_  *
//* | '_ \ / _ \/ _` | '__| __| '_ \ / _ \/ _` | __| *
//* | | | |  __/ (_| | |  | |_| |_) |  __/ (_| | |_  *
//* |_| |_|\___|\__,_|_|   \__|_.__/ \___|\__,_|\__| *
//*                                                  *
//===- unittests/core/test_heartbeat.cpp ----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file test_heartbeat.cpp

#include "heartbeat.hpp"
#include <gmock/gmock.h>

namespace {
    struct callback_base {
        virtual ~callback_base () {}
        virtual void callback (pstore::heartbeat::key_type) {}
    };
    class mock_callback final : public callback_base {
    public:
        MOCK_METHOD1 (callback, void (pstore::heartbeat::key_type));
    };

    class HeartbeatAttachDetach : public ::testing::Test {
    public:
        void SetUp () final {}
        void TearDown () final {}

    protected:
        void attach (int const * const v);
        void detach (int const * const v);

        mock_callback callback_;
        pstore::heartbeat::worker_thread worker_;
    };

    void HeartbeatAttachDetach::attach (int const * const v) {
        using namespace std::placeholders;
        auto const key = pstore::heartbeat::to_key_type (v);
        worker_.attach (key, std::bind (&mock_callback::callback, &callback_, _1));
    }

    void HeartbeatAttachDetach::detach (int const * const v) {
        auto const key = pstore::heartbeat::to_key_type (v);
        worker_.detach (key);
    }
} // namespace

TEST_F (HeartbeatAttachDetach, SingleAttach) {
    int dummy = 42;
    EXPECT_CALL (callback_, callback (pstore::heartbeat::to_key_type (&dummy))).Times (2);
    this->attach (&dummy);
    worker_.step ();
}

TEST_F (HeartbeatAttachDetach, MultipleAttach) {
    int dummy = 42;
    EXPECT_CALL (callback_, callback (pstore::heartbeat::to_key_type (&dummy))).Times (3);
    this->attach (&dummy);
    this->attach (&dummy);
    worker_.step ();
}

TEST_F (HeartbeatAttachDetach, SingleAttachDetach) {
    int dummy = 42;
    EXPECT_CALL (callback_, callback (pstore::heartbeat::to_key_type (&dummy))).Times (1);
    this->attach (&dummy);
    this->detach (&dummy);
    worker_.step ();
}

TEST_F (HeartbeatAttachDetach, AttachTwo) {
    int first = 27;
    int second = 31;
    EXPECT_CALL (callback_, callback (pstore::heartbeat::to_key_type (&first))).Times (2);
    EXPECT_CALL (callback_, callback (pstore::heartbeat::to_key_type (&second))).Times (2);
    this->attach (&first);
    this->attach (&second);
    worker_.step ();
}

TEST_F (HeartbeatAttachDetach, AttachTwoDetachOne) {
    int first = 27;
    int second = 31;
    EXPECT_CALL (callback_, callback (pstore::heartbeat::to_key_type (&first))).Times (3);
    EXPECT_CALL (callback_, callback (pstore::heartbeat::to_key_type (&second))).Times (2);
    this->attach (&first);
    this->attach (&second);
    worker_.step ();
    this->detach (&second);
    worker_.step ();
}
