//*  _                     _   _                _    *
//* | |__   ___  __ _ _ __| |_| |__   ___  __ _| |_  *
//* | '_ \ / _ \/ _` | '__| __| '_ \ / _ \/ _` | __| *
//* | | | |  __/ (_| | |  | |_| |_) |  __/ (_| | |_  *
//* |_| |_|\___|\__,_|_|   \__|_.__/ \___|\__,_|\__| *
//*                                                  *
//===- lib/pstore/heartbeat.hpp -------------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
/// \file heartbeat.hpp
/// \brief Provides an asynchonous background "heartbeat" for the data store.

#ifndef PSTORE_HEARTBEAT_HPP
#define PSTORE_HEARTBEAT_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace pstore {

    class heartbeat {
    public:
        static std::shared_ptr <heartbeat> get ();

        ~heartbeat ();

        heartbeat (heartbeat const &) = delete;
        heartbeat & operator= (heartbeat const &) = delete;

        /// A key_type value is used to distinguish between different callbacks that are
        /// attached to the heartbeat thread. When attaching a callback, provide a unique
        /// that identifies it. This same value is passed to the function when it is called
        /// and may be used as the argument to detach() when it is time so stop that
        /// callback being invoked.
        using key_type = std::uintptr_t;

        /// A small convenience function which will convert a pointer to key_type.
        template <typename T>
        static key_type to_key_type (T * t) {
            static_assert (sizeof (key_type) <= sizeof (t), "heartbeat::key_type is too small");
            return reinterpret_cast <key_type> (t);
        }

        using callback = std::function<void(key_type)>;
        void attach (key_type key, callback cb);
        void detach (key_type key);

        /// Stops the heartbeat thread.
        void stop ();

        /// \note This class is in the public interface to enable it to be unit tested.
        class worker_thread {
        public:
            worker_thread ();
            ~worker_thread () = default;

            // Move.
            worker_thread (worker_thread &&) = default;
            worker_thread & operator= (worker_thread &&) = default;

            // No copying or assignment
            worker_thread (worker_thread const &) = delete;
            worker_thread & operator= (worker_thread const &) = delete;

            void attach (heartbeat::key_type key, callback cb);
            void detach (heartbeat::key_type key);

            /// This is the thread entry point.
            void run () noexcept;

            /// Executes a single invocation of each of the attached callbacks. This is exposed for
            /// unit testing.
            /// \note Except for during unit tests, the #mut_ mutex must be held on entry to this
            /// function.
            void step () const;

            /// Instructs the worker thread to exit on its next iteration. The condition variable is
            /// signalled to wait up the thread.
            void stop () noexcept;

        private:
            using duration_type = std::chrono::milliseconds;
            /// The duration used for the worker thread's sleep time when no callbacks are attached.
            static duration_type const delay_time_;
            /// The duration used for the worker thread's sleep time when one or more callbacks are attached.
            static duration_type const max_time_;

            /// The time for which the thread will sleep before waking to perform a step of the
            /// attached callbacks. This is a pointer to either #max_time_ or #delay_time_ depending
            /// on whether any callbacks are attached. It is a pointer rather than a duration_type
            /// to maximize the chance that the atomic type will not introduce another mutex.
            duration_type const * sleep_time_;

            /// True when the thread is to exit on its next iteration.
            bool done_ = false;

            /// Protects access to the callbacks_ container (in conjunction with the #cv_ condition
            /// variable).
            std::mutex mut_;
            /// Protects access to the callbacks_ container (in conjunction with the #mut_ mutex).
            std::condition_variable cv_;

            /// The container which associates keys with their corresponding callback. This holds
            /// the collection of operations performed at each beat of the heart.
            std::unordered_map<heartbeat::key_type, heartbeat::callback> callbacks_;
        };

    private:
        /// A private constructor ensures that the get() method must be used to instantiate this
        /// class.
        heartbeat () noexcept = default;

        struct state {
            worker_thread worker;
            std::thread thread;
        };
        std::unique_ptr <state> state_;
    };
} // namespace pstore
#endif // PSTORE_HEARTBEAT_HPP
// eof: lib/pstore/heartbeat.hpp
