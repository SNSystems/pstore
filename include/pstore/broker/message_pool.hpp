//*                                                               _  *
//*  _ __ ___   ___  ___ ___  __ _  __ _  ___   _ __   ___   ___ | | *
//* | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ | '_ \ / _ \ / _ \| | *
//* | | | | | |  __/\__ \__ \ (_| | (_| |  __/ | |_) | (_) | (_) | | *
//* |_| |_| |_|\___||___/___/\__,_|\__, |\___| | .__/ \___/ \___/|_| *
//*                                |___/       |_|                   *
//===- include/pstore/broker/message_pool.hpp -----------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file message_pool.hpp
/// \brief Stores messages after receipt and before processing by the command thread.
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

#include "pstore/brokerface/message_type.hpp"

namespace pstore {
    namespace broker {

        class message_pool {
        public:
            message_pool () = default;
            message_pool (message_pool const &) = delete;
            message_pool (message_pool &&) = delete;

            ~message_pool () noexcept = default;

            message_pool & operator= (message_pool const &) = delete;
            message_pool & operator= (message_pool &&) = delete;

            void return_to_pool (brokerface::message_ptr && ptr);
            brokerface::message_ptr get_from_pool ();

        private:
            std::mutex mut_;
            std::queue<brokerface::message_ptr> queue_;
        };

        inline void message_pool::return_to_pool (brokerface::message_ptr && ptr) {
            std::unique_lock<std::mutex> const lock (mut_);
            PSTORE_ASSERT (ptr.get () != nullptr);
            queue_.push (std::move (ptr));
        }

        inline brokerface::message_ptr message_pool::get_from_pool () {
            std::unique_lock<std::mutex> lock (mut_);
            if (queue_.empty ()) {
                lock.unlock ();
                return std::make_unique<brokerface::message_type> ();
            }

            auto res = std::move (queue_.front ());
            queue_.pop ();
            return res;
        }

        extern message_pool pool;

    } // end namespace broker
} // end namespace pstore

#endif // PSTORE_BROKER_MESSAGE_POOL_HPP
