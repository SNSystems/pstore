//*                                                                          *
//*  _ __ ___   ___  ___ ___  __ _  __ _  ___    __ _ _   _  ___ _   _  ___  *
//* | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \  / _` | | | |/ _ \ | | |/ _ \ *
//* | | | | | |  __/\__ \__ \ (_| | (_| |  __/ | (_| | |_| |  __/ |_| |  __/ *
//* |_| |_| |_|\___||___/___/\__,_|\__, |\___|  \__, |\__,_|\___|\__,_|\___| *
//*                                |___/           |_|                       *
//===- include/pstore/broker/message_queue.hpp ----------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file message_queue.hpp

#ifndef PSTORE_BROKER_MESSAGE_QUEUE_HPP
#define PSTORE_BROKER_MESSAGE_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

namespace pstore {
    namespace broker {

        template <typename T>
        class message_queue {
        public:
            void push (T && message);
            T pop ();
            void clear ();

        private:
            std::mutex mut_;
            std::condition_variable cv;
            std::queue<T> queue_;
        };

        template <typename T>
        void message_queue<T>::push (T && message) {
            std::unique_lock<decltype (mut_)> lock (mut_);
            queue_.push (std::move (message));
            cv.notify_one ();
        }

        template <typename T>
        T message_queue<T>::pop () {
            std::unique_lock<decltype (mut_)> lock (mut_);
            cv.wait (lock, [this] () { return !queue_.empty (); });
            T res = std::move (queue_.front ());
            queue_.pop ();
            return res;
        }

        template <typename T>
        void message_queue<T>::clear () {
            std::unique_lock<decltype (mut_)> lock (mut_);
            while (!queue_.empty ()) {
                queue_.pop ();
            }
        }

    } // namespace broker
} // namespace pstore

#endif // PSTORE_BROKER_MESSAGE_QUEUE_HPP
