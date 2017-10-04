//*                                                _  *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| *
//*                                                   *
//===- include/broker/command.hpp -----------------------------------------===//
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
/// \file command.hpp

#ifndef PSTORE_BROKER_COMMAND_HPP
#define PSTORE_BROKER_COMMAND_HPP

#include <atomic>
#include <mutex>

// pstore includes
#include "pstore_broker_intf/message_type.hpp"

// local includes
#include "./message_queue.hpp"
#include "./parser.hpp"

namespace pstore {
    namespace broker {
        class fifo_path;
    } // namespace broker
} // namespace pstore

class recorder;
class scavenger;

/// A class which is responsible for managing the command queue and which provides the
/// thread_entry() funtion for pulling commands from the queue and executing them.
class command_processor {
public:
    explicit command_processor (unsigned const num_read_threads)
            : num_read_threads_{num_read_threads} {}

    // No copying or assignment.
    command_processor (command_processor const &) = delete;
    command_processor & operator= (command_processor const &) = delete;

    void thread_entry (pstore::broker::fifo_path const & fifo);

    void attach_scavenger (std::shared_ptr<scavenger> & scav) {
        scavenger_.set (scav);
    }

    /// Pushes a command onto the end of the command queue. The command is recorded if `record_file`
    /// is not null.
    /// \param cmd  The message to which this parameter points is moved to the end of the command
    /// queue.
    /// \param record_file  If not null, this object is used to record the command.
    void push_command (pstore::broker::message_ptr && cmd, recorder * const record_file);
    void clear_queue ();

    void scavenge ();

private:
    std::atomic<bool> commands_done_{false};

    template <typename T>
    class atomic_weak_ptr {
    public:
        std::shared_ptr<T> get () {
            std::lock_guard<decltype (mut_)> lock{mut_};
            return ptr_.lock ();
        }

        void set (std::shared_ptr<T> & t) {
            std::lock_guard<decltype (mut_)> lock{mut_};
            ptr_ = t;
        }
        void set (std::weak_ptr<T> & t) {
            std::lock_guard<decltype (mut_)> lock{mut_};
            ptr_ = t;
        }

    private:
        std::mutex mut_;
        std::weak_ptr<T> ptr_;
    };

    atomic_weak_ptr<scavenger> scavenger_;

    message_queue<pstore::broker::message_ptr> messages_;

    std::mutex cmds_mut_;
    broker::partial_cmds cmds_;

    unsigned const num_read_threads_;

    void process_command (pstore::broker::fifo_path const & fifo,
                          pstore::broker::message_type const & msg);

    auto parse (pstore::broker::message_type const & msg)
        -> std::unique_ptr<broker::broker_command>;
};

#endif // PSTORE_BROKER_COMMAND_HPP
// eof: include/broker/command.hpp
