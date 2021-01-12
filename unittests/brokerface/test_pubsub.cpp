//*              _               _      *
//*  _ __  _   _| |__  ___ _   _| |__   *
//* | '_ \| | | | '_ \/ __| | | | '_ \  *
//* | |_) | |_| | |_) \__ \ |_| | |_) | *
//* | .__/ \__,_|_.__/|___/\__,_|_.__/  *
//* |_|                                 *
//===- unittests/brokerface/test_pubsub.cpp -------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
        virtual ~received_base () = default;
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
