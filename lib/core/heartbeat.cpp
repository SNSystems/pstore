//*  _                     _   _                _    *
//* | |__   ___  __ _ _ __| |_| |__   ___  __ _| |_  *
//* | '_ \ / _ \/ _` | '__| __| '_ \ / _ \/ _` | __| *
//* | | | |  __/ (_| | |  | |_| |_) |  __/ (_| | |_  *
//* |_| |_|\___|\__,_|_|   \__|_.__/ \___|\__,_|\__| *
//*                                                  *
//===- lib/core/heartbeat.cpp ---------------------------------------------===//
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
/// \file heartbeat.cpp

#include "heartbeat.hpp"

#include "pstore/os/thread.hpp"
#include "pstore/support/portab.hpp"

namespace pstore {

    // ******************************
    // * heartbeat :: worker thread *
    // ******************************
    heartbeat::worker_thread::duration_type const heartbeat::worker_thread::delay_time_ =
        std::chrono::seconds (1);

    heartbeat::worker_thread::duration_type const heartbeat::worker_thread::max_time_ =
        duration_type::max ();

    heartbeat::worker_thread::worker_thread ()
            : sleep_time_ (&max_time_) {}

    void heartbeat::worker_thread::attach (heartbeat::key_type const key, callback const cb) {
        // Pre-emptively invoke the callback. This ensures that it is called at least once
        // even if the worker thread is not sheduled before it exits.
        cb (key);

        std::lock_guard<std::mutex> const lock{mut_};
        callbacks_[key] = cb;
        sleep_time_ = &delay_time_;
        cv_.notify_all ();
    }

    void heartbeat::worker_thread::detach (heartbeat::key_type const key) {
        std::lock_guard<std::mutex> const lock{mut_};
        callbacks_.erase (key);
        if (callbacks_.empty ()) {
            sleep_time_ = &max_time_;
        }
    }

    void heartbeat::worker_thread::step () const {
        for (auto const & kv : callbacks_) {
            kv.second (kv.first);
        }
    }

    void heartbeat::worker_thread::run () noexcept {
        PSTORE_TRY {
            std::unique_lock<std::mutex> lock{mut_};
            while (!done_) {
                this->step ();
                cv_.wait_for (lock, *sleep_time_);
            }
        }
        // clang-format off
        PSTORE_CATCH (..., {
            // Something bad happened. What's a girl to do?
        })
        // clang-format on
    }

    void heartbeat::worker_thread::stop () noexcept {
        std::lock_guard<std::mutex> const lock (mut_);
        done_ = true;
        cv_.notify_all ();
    }


    //*************
    //* heartbeat *
    //*************
    std::shared_ptr<heartbeat> heartbeat::get () {
        static std::shared_ptr<heartbeat> const t (new heartbeat);
        return t;
    }

    heartbeat::~heartbeat () {
        if (state_) {
            state_->worker.stop ();
            state_->thread.join ();
        }
    }

    void heartbeat::attach (key_type const key, callback const cb) {
        if (!state_) {
            state_ = std::make_unique<state> ();
            auto & w = state_->worker;
            state_->thread = std::thread ([&w] () {
                threads::set_name ("heartbeat");
                w.run ();
            });
        }
        state_->worker.attach (key, cb);
    }

    void heartbeat::detach (key_type const key) {
        if (state_) {
            state_->worker.detach (key);
        }
    }

} // namespace pstore
