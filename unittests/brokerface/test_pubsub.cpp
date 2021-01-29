//*              _               _      *
//*  _ __  _   _| |__  ___ _   _| |__   *
//* | '_ \| | | | '_ \/ __| | | | '_ \  *
//* | |_) | |_| | |_) \__ \ |_| | |_) | *
//* | .__/ \__,_|_.__/|___/\__,_|_.__/  *
//* |_|                                 *
//===- unittests/brokerface/test_pubsub.cpp -------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/brokerface/pubsub.hpp"

#include <condition_variable>
#include <thread>

#include <gmock/gmock.h>

using namespace std::string_literals;

namespace {

    class counter {
    public:
        counter () = default;
        counter (counter const &) = delete;
        counter (counter &&) = delete;
        counter & operator= (counter const &) = delete;
        counter & operator= (counter &&) = delete;

        int increment ();
        void wait_for_value (int v) const;

    private:
        mutable std::mutex mut_;
        mutable std::condition_variable cv_;

        int count_ = 0;
    };

    int counter::increment () {
        std::lock_guard<std::mutex> _{mut_};
        ++count_;
        cv_.notify_one ();
        return count_;
    }

    void counter::wait_for_value (int v) const {
        std::unique_lock<std::mutex> lock{mut_};
        while (count_ < v) {
            cv_.wait (lock);
        }
    }

    class received_base {
    public:
        virtual ~received_base () noexcept = default;
        virtual void call (std::string const & message) const = 0;
    };

    class mock_received : public received_base {
    public:
        MOCK_CONST_METHOD1 (call, void (std::string const &));
    };

} // end anonymous namespace


TEST (PubSub, PubSub) {
    std::condition_variable cv;
    pstore::brokerface::channel<decltype (cv)> chan{&cv};

    counter listening_counter;
    counter received_counter;
    mock_received received;

    EXPECT_CALL (received, call ("message 1"s));
    EXPECT_CALL (received, call ("message 2"s));

    std::unique_ptr<pstore::brokerface::subscriber<decltype (cv)>> sub = chan.new_subscriber ();

    std::thread thread{[&] () {
        listening_counter.increment ();
        while (pstore::maybe<std::string> const message = sub->listen ()) {
            received_counter.increment ();
            received.call (*message);
        }
    }};

    // Wait for my subscriber to get to the point that it's listening.
    listening_counter.wait_for_value (1);

    // Now post some messages to the channel.
    chan.publish ("message 1");
    chan.publish ([] (char const * s) { return std::string{s}; }, "message 2");

    received_counter.wait_for_value (2);
    sub->cancel ();
    thread.join ();
}

namespace {

    class int_message_base {
    public:
        virtual ~int_message_base () noexcept = default;
        virtual std::string call (int a) const = 0;
    };

    class int_message : public int_message_base {
    public:
        MOCK_CONST_METHOD1 (call, std::string (int));
    };

} // end anonymous namespace

TEST (PubSub, PublishWithNooneListening) {
    using testing::_;
    int_message fn;
    EXPECT_CALL (fn, call (_)).Times (0);
    std::condition_variable cv;
    pstore::brokerface::channel<decltype (cv)> chan{&cv};
    chan.publish ([&fn] (int a) { return fn.call (a); }, 7);
}
