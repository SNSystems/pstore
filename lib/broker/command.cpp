//*                                                _  *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| *
//*                                                   *
//===- lib/broker/command.cpp ---------------------------------------------===//
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
/// \file command.cpp

#include "broker/command.hpp"

#include <algorithm>
#include <condition_variable>
#include <cstdio>
#include <iomanip>
#include <mutex>
#include <queue>

// platform includes
#ifndef _WIN32
#include <unistd.h>
#endif

#include "pstore_broker_intf/fifo_path.hpp"
#include "pstore_broker_intf/writer.hpp"
#include "pstore_support/logging.hpp"
#include "broker/gc.hpp"
#include "broker/globals.hpp"
#include "broker/message_pool.hpp"
#include "broker/quit.hpp"
#include "broker/recorder.hpp"

// process_command
// ~~~~~~~~~~~~~~~
void command_processor::process_command (pstore::broker::fifo_path const & fifo,
                                         pstore::broker::message_type const & msg) {

    auto const command = this->parse (msg);
    if (broker::broker_command const * const c = command.get ()) {
        pstore::logging::log (pstore::logging::priority::info, "verb:", c->verb, " path:",
                              c->path.substr (0, 32));

        if (c->verb == "SUICIDE") {
            std::shared_ptr<scavenger> scav = scavenger_.get ();
            shutdown (this, scav.get (), -1, num_read_threads_);
        } else if (c->verb == "_QUIT") {
            // Shut down a single pipe-reader thread if the 'done' flag has been set by a call to
            // shutdown().
            if (!done) {
                pstore::logging::log (pstore::logging::priority::info,
                                      "_QUIT ignored: not shutting down");
            } else {
                pstore::logging::log (pstore::logging::priority::info, "waking one reader thread");

                // Post a message back to one of the read-loop threads. Since 'done' is now true, it
                // will exit as soon as it is woken up by the presence of the new data (the content
                // of the message doesn't matter at all).
                pstore::broker::writer wr (fifo);
                wr.write (pstore::broker::message_type{}, true/*issue error on timeout*/);
            }
        } else if (c->verb == "_CQUIT") {
            commands_done_ = true;
        } else if (c->verb == "GC") {
            broker::start_vacuum (c->path);
        } else if (c->verb == "ECHO") {
            std::printf ("ECHO:%s\n", c->path.c_str ());
        } else {
            pstore::logging::log (pstore::logging::priority::error, "unknown verb:", c->verb);
        }
    }
}

// parse
// ~~~~~
auto command_processor::parse (pstore::broker::message_type const & msg)
    -> std::unique_ptr<broker::broker_command> {
    std::lock_guard<decltype (cmds_mut_)> lock{cmds_mut_};
    return broker::parse (msg, cmds_);
}

// thread_entry
// ~~~~~~~~~~~~
void command_processor::thread_entry (pstore::broker::fifo_path const & fifo) {
    try {
        pstore::logging::log (pstore::logging::priority::info, "waiting for commands");
        while (!commands_done_) {
            pstore::broker::message_ptr msg = messages_.pop ();
            assert (msg);
            this->process_command (fifo, *msg);
            pool.return_to_pool (std::move (msg));
        }
    } catch (std::exception const & ex) {
        pstore::logging::log (pstore::logging::priority::error, "An error occurred: ", ex.what ());
    } catch (...) {
        pstore::logging::log (pstore::logging::priority::error, "Unknown error");
    }
    pstore::logging::log (pstore::logging::priority::info, "exiting command thread");
}

// push_command
// ~~~~~~~~~~~~
void command_processor::push_command (pstore::broker::message_ptr && cmd,
                                      recorder * const record_file) {
    if (record_file) {
        record_file->record (*cmd);
    }
    messages_.push (std::move (cmd));
}

// clear_queue
// ~~~~~~~~~~~
void command_processor::clear_queue () {
    messages_.clear ();
}

// scavenge
// ~~~~~~~~
void command_processor::scavenge () {
    pstore::logging::log (pstore::logging::priority::info, "scavenging zombie commands");

    // TODO: make this time threshold user configurable
    constexpr auto delete_threshold = std::chrono::seconds (4 * 60 * 60);

    // After this function is complete, no partial messages older than 'earliest time' will be in
    // the partial command map (cmds_).
    auto const earliest_time = std::chrono::system_clock::now () - delete_threshold;

    // Remove all partial messages where the last piece of the message arrived before earliest_time.
    // It's most likely that the sending process gave up/crashed/lost interest before sending the
    // complete message.
    std::lock_guard<decltype (cmds_mut_)> lock{cmds_mut_};
    for (auto it = std::begin (cmds_); it != std::end (cmds_);) {
        auto const arrival_time = it->second.arrive_time_;
        if (arrival_time < earliest_time) {
            auto const t = std::chrono::system_clock::to_time_t (arrival_time);
            pstore::logging::log (pstore::logging::priority::info,
                                  "deleted old partial message (arrived ",
                                  std::put_time (std::localtime (&t), "%F %T"), ")");
            it = cmds_.erase (it);
        } else {
            ++it;
        }
    }
}

// eof: lib/broker/command.cpp
