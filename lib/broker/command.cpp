//*                                                _  *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| *
//*                                                   *
//===- lib/broker/command.cpp ---------------------------------------------===//
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

// ctor
// ~~~~
command_processor::command_processor (unsigned const num_read_threads)
        : num_read_threads_{num_read_threads} {

    assert (std::is_sorted (std::begin (commands_), std::end (commands_), command_entry_compare));
}

// suicide
// ~~~~~~~
void command_processor::suicide (pstore::broker::fifo_path const &,
                                 broker::broker_command const &) {
    std::shared_ptr<scavenger> scav = scavenger_.get ();
    shutdown (this, scav.get (), -1, num_read_threads_);
}

// quit
// ~~~~
void command_processor::quit (pstore::broker::fifo_path const & fifo,
                              broker::broker_command const &) {
    // Shut down a single pipe-reader thread if the 'done' flag has been set by a call to
    // shutdown().
    if (!done) {
        this->log ("_QUIT ignored: not shutting down");
    } else {
        this->log ("waking one reader thread");

        // Post a message back to one of the read-loop threads. Since 'done' is now true, it
        // will exit as soon as it is woken up by the presence of the new data (the content
        // of the message doesn't matter at all).
        pstore::broker::writer wr (fifo);
        wr.write (pstore::broker::message_type{}, true /*issue error on timeout*/);
    }
}

// cquit
// ~~~~~
void command_processor::cquit (pstore::broker::fifo_path const &, broker::broker_command const &) {
    commands_done_ = true;
}

// gc
// ~~
void command_processor::gc (pstore::broker::fifo_path const &, broker::broker_command const & c) {
    broker::start_vacuum (c.path);
}

// echo
// ~~~~
void command_processor::echo (pstore::broker::fifo_path const &, broker::broker_command const & c) {
    std::printf ("ECHO:%s\n", c.path.c_str ());
}

// nop
// ~~~
void command_processor::nop (pstore::broker::fifo_path const &, broker::broker_command const &) {}

// unknown
// ~~~~~~~
void command_processor::unknown (broker::broker_command const & c) const {
    pstore::logging::log (pstore::logging::priority::error, "unknown verb:", c.verb);
}

namespace {
    template <typename T, std::size_t N>
    constexpr std::size_t array_size (T const (&)[N]) {
        return N;
    }
} // anonymous namespace

// log
// ~~~
void command_processor::log (broker::broker_command const & c) const {
    constexpr char const verb[] = "verb:";
    constexpr auto verb_length = array_size (verb) - 1;
    constexpr char const path[] = " path:";
    constexpr auto path_length = array_size (path) - 1;
    constexpr auto max_path_length = std::size_t{32};

    std::string message;
    message.reserve (verb_length + c.verb.length () + path_length + max_path_length);
    message = verb + c.verb + path + c.path.substr (0, max_path_length);
    pstore::logging::log (pstore::logging::priority::info, message.c_str ());
}

void command_processor::log (pstore::gsl::czstring str) const {
    pstore::logging::log (pstore::logging::priority::info, str);
}

std::array<command_processor::command_entry, 6> const command_processor::commands_{{
    command_processor::command_entry ("ECHO", &command_processor::echo),
    command_processor::command_entry ("GC", &command_processor::gc),
    command_processor::command_entry ("NOP", &command_processor::nop),
    command_processor::command_entry ("SUICIDE",
                                      &command_processor::suicide), // initiate the broker shutdown.
    command_processor::command_entry (
        "_CQUIT", &command_processor::cquit), // exit this command processor thread.
    command_processor::command_entry (
        "_QUIT", &command_processor::quit), //  shut down a single pipe-reader thread.
}};

// process_command
// ~~~~~~~~~~~~~~~
void command_processor::process_command (pstore::broker::fifo_path const & fifo,
                                         pstore::broker::message_type const & msg) {
    auto const command = this->parse (msg);
    if (broker::broker_command const * const c = command.get ()) {
        this->log (*c);
        auto it =
            std::lower_bound (std::begin (commands_), std::end (commands_),
                              command_entry (c->verb.c_str (), nullptr), command_entry_compare);
        if (it != std::end (commands_) && c->verb == std::get<0> (*it)) {
            std::get<1> (*it) (this, fifo, *c);
        } else {
            this->unknown (*c);
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
        pstore::logging::log (pstore::logging::priority::info, "Waiting for commands");
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
    pstore::logging::log (pstore::logging::priority::info, "Exiting command thread");
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
    pstore::logging::log (pstore::logging::priority::info, "Scavenging zombie commands");

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

            static constexpr auto time_size = std::size_t{100};
            char time_str[time_size];
            bool got_time = false;

            if (std::tm const * tm = std::gmtime (&t)) {
                // strftime() returns 0 if the result doesn't fit in the provided buffer;
                // the contents are undefined.
                if (std::strftime (time_str, time_size, "%FT%TZ", tm) != 0) {
                    got_time = true;
                }
            }
            if (!got_time) {
                std::strncpy (time_str, "(unknown)", time_size);
            }

            // Guarantee that the string is null terminated.
            time_str[time_size - 1] = '\0';

            pstore::logging::log (pstore::logging::priority::info,
                                  "Deleted old partial message. Arrived ", time_str);
            it = cmds_.erase (it);
        } else {
            ++it;
        }
    }
}

// eof: lib/broker/command.cpp
