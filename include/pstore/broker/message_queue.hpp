//*                                                                          *
//*  _ __ ___   ___  ___ ___  __ _  __ _  ___    __ _ _   _  ___ _   _  ___  *
//* | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \  / _` | | | |/ _ \ | | |/ _ \ *
//* | | | | | |  __/\__ \__ \ (_| | (_| |  __/ | (_| | |_| |  __/ |_| |  __/ *
//* |_| |_| |_|\___||___/___/\__,_|\__, |\___|  \__, |\__,_|\___|\__,_|\___| *
//*                                |___/           |_|                       *
//===- include/pstore/broker/message_queue.hpp ----------------------------===//
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
            cv.wait (lock, [this]() { return queue_.size () > 0; });
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
// eof: include/pstore/broker/message_queue.hpp
