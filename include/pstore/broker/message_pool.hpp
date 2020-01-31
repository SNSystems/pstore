//*                                                               _  *
//*  _ __ ___   ___  ___ ___  __ _  __ _  ___   _ __   ___   ___ | | *
//* | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ | '_ \ / _ \ / _ \| | *
//* | | | | | |  __/\__ \__ \ (_| | (_| |  __/ | |_) | (_) | (_) | | *
//* |_| |_| |_|\___||___/___/\__,_|\__, |\___| | .__/ \___/ \___/|_| *
//*                                |___/       |_|                   *
//===- include/pstore/broker/message_pool.hpp -----------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
/// \file message_pool.hpp
///
/// Buffer Life-Cycle
/// -----------------
/// To reduce the number of memory allocations performed by the performance-sensitive read
/// loop thread -- which is responsible for accepting commands from clients -- command
/// buffers are recycled when they have been processed. The basic flow is as follows:
///
/// - The read loop thread draws a message buffer from the pool before beginning an asynchronous
/// read from the named pipe.
/// - If the buffer pool is exhausted, then a new command buffer instance is allocated.
/// - Once the asynchronous read has completed, the message buffer is moved to the command queue.
/// - The command thread draws
///
/// \image html buffer_life_cycle.svg

#ifndef PSTORE_BROKER_MESSAGE_POOL_HPP
#define PSTORE_BROKER_MESSAGE_POOL_HPP

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

#include "pstore/broker_intf/message_type.hpp"

namespace pstore {
    namespace broker {

        class message_pool {
        public:
            void return_to_pool (message_ptr && ptr);
            message_ptr get_from_pool ();

        private:
            std::mutex mut_;
            std::queue<message_ptr> queue_;
        };

        inline void message_pool::return_to_pool (message_ptr && ptr) {
            std::unique_lock<std::mutex> const lock (mut_);
            assert (ptr.get () != nullptr);
            queue_.push (std::move (ptr));
        }

        inline message_ptr message_pool::get_from_pool () {
            std::unique_lock<std::mutex> lock (mut_);
            if (queue_.empty ()) {
                lock.unlock ();
                return std::make_unique<message_type> ();
            }

            auto res = std::move (queue_.front ());
            queue_.pop ();
            return res;
        }

        extern message_pool pool;

    } // namespace broker
} // namespace pstore

#endif // PSTORE_BROKER_MESSAGE_POOL_HPP
