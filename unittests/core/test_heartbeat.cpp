//*  _                     _   _                _    *
//* | |__   ___  __ _ _ __| |_| |__   ___  __ _| |_  *
//* | '_ \ / _ \/ _` | '__| __| '_ \ / _ \/ _` | __| *
//* | | | |  __/ (_| | |  | |_| |_) |  __/ (_| | |_  *
//* |_| |_|\___|\__,_|_|   \__|_.__/ \___|\__,_|\__| *
//*                                                  *
//===- unittests/core/test_heartbeat.cpp ----------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
/// \file test_heartbeat.cpp

#include "heartbeat.hpp"
#include <gmock/gmock.h>

namespace {
    struct callback_base {
        virtual ~callback_base () {}
        virtual void callback (pstore::heartbeat::key_type) {}
    };
    struct mock_callback final : public callback_base {
        MOCK_METHOD1 (callback, void(pstore::heartbeat::key_type));
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

