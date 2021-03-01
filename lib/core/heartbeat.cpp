//===- lib/core/heartbeat.cpp ---------------------------------------------===//
//*  _                     _   _                _    *
//* | |__   ___  __ _ _ __| |_| |__   ___  __ _| |_  *
//* | '_ \ / _ \/ _` | '__| __| '_ \ / _ \/ _` | __| *
//* | | | |  __/ (_| | |  | |_| |_) |  __/ (_| | |_  *
//* |_| |_|\___|\__,_|_|   \__|_.__/ \___|\__,_|\__| *
//*                                                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
