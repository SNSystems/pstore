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

#include "pstore/broker/command.hpp"

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

#include "pstore/broker/gc.hpp"
#include "pstore/broker/globals.hpp"
#include "pstore/broker/message_pool.hpp"
#include "pstore/broker/quit.hpp"
#include "pstore/broker/recorder.hpp"
#include "pstore/broker_intf/fifo_path.hpp"
#include "pstore/broker_intf/writer.hpp"
#include "pstore/support/logging.hpp"

namespace {
    template <typename T, std::size_t N>
    constexpr std::size_t array_size (T const (&)[N]) {
        return N;
    }
} // anonymous namespace

namespace pstore {
    namespace broker {

        // ctor
        // ~~~~
        command_processor::command_processor (unsigned const num_read_threads,
                                              std::weak_ptr<self_client_connection> status_client)
                : status_client_{std::move (status_client)}
                , num_read_threads_{num_read_threads} {

            assert (std::is_sorted (std::begin (commands_), std::end (commands_),
                                    command_entry_compare));
        }

        // suicide
        // ~~~~~~~
        void command_processor::suicide (fifo_path const &, broker_command const &) {
            std::shared_ptr<scavenger> scav = scavenger_.get ();
            std::shared_ptr<self_client_connection> conn = status_client_.lock ();
            shutdown (this, scav.get (), sig_self_quit, num_read_threads_, conn.get ());
        }

        // quit
        // ~~~~
        void command_processor::quit (fifo_path const & fifo, broker_command const &) {
            // Shut down a single pipe-reader thread if the 'done' flag has been set by a call to
            // shutdown().
            if (!done) {
                this->log ("_QUIT ignored: not shutting down");
            } else {
                this->log ("waking one reader thread");

                // Post a message back to one of the read-loop threads. Since 'done' is now true, it
                // will exit as soon as it is woken up by the presence of the new data (the content
                // of the message doesn't matter at all).
                writer wr (fifo);
                wr.write (message_type{}, true /*issue error on timeout*/);
            }
        }

        // cquit
        // ~~~~~
        void command_processor::cquit (fifo_path const &, broker_command const &) {
            commands_done_ = true;
        }

        // gc
        // ~~
        void command_processor::gc (fifo_path const &, broker_command const & c) {
            start_vacuum (c.path);
        }

        // echo
        // ~~~~
        void command_processor::echo (fifo_path const &, broker_command const & c) {
            std::printf ("ECHO:%s\n", c.path.c_str ());
        }

        // nop
        // ~~~
        void command_processor::nop (fifo_path const &, broker_command const &) {}

        // unknown
        // ~~~~~~~
        void command_processor::unknown (broker_command const & c) const {
            logging::log (logging::priority::error, "unknown verb:", c.verb);
        }

        // log
        // ~~~
        void command_processor::log (broker_command const & c) const {
            constexpr char const verb[] = "verb:";
            constexpr auto verb_length = array_size (verb) - 1;
            constexpr char const path[] = " path:";
            constexpr auto path_length = array_size (path) - 1;
            constexpr auto max_path_length = std::size_t{32};

            std::string message;
            message.reserve (verb_length + c.verb.length () + path_length + max_path_length);
            message = verb + c.verb + path + c.path.substr (0, max_path_length);
            logging::log (logging::priority::info, message.c_str ());
        }

        void command_processor::log (gsl::czstring str) const {
            logging::log (logging::priority::info, str);
        }

        std::array<command_processor::command_entry, 6> const command_processor::commands_{{
            command_processor::command_entry ("ECHO", &command_processor::echo),
            command_processor::command_entry ("GC", &command_processor::gc),
            command_processor::command_entry ("NOP", &command_processor::nop),
            command_processor::command_entry (
                "SUICIDE",
                &command_processor::suicide), // initiate the broker shutdown.
            command_processor::command_entry (
                "_CQUIT", &command_processor::cquit), // exit this command processor thread.
            command_processor::command_entry (
                "_QUIT", &command_processor::quit), //  shut down a single pipe-reader thread.
        }};

        // process_command
        // ~~~~~~~~~~~~~~~
        void command_processor::process_command (fifo_path const & fifo, message_type const & msg) {
            auto const command = this->parse (msg);
            if (broker_command const * const c = command.get ()) {
                this->log (*c);
                auto it = std::lower_bound (std::begin (commands_), std::end (commands_),
                                            command_entry (c->verb.c_str (), nullptr),
                                            command_entry_compare);
                if (it != std::end (commands_) && c->verb == std::get<0> (*it)) {
                    std::get<1> (*it) (this, fifo, *c);
                } else {
                    this->unknown (*c);
                }
            }
        }

        // parse
        // ~~~~~
        auto command_processor::parse (message_type const & msg)
            -> std::unique_ptr<broker_command> {
            std::lock_guard<decltype (cmds_mut_)> lock{cmds_mut_};
            return ::pstore::broker::parse (msg, cmds_);
        }

        // thread_entry
        // ~~~~~~~~~~~~
        void command_processor::thread_entry (fifo_path const & fifo) {
            try {
                logging::log (logging::priority::info, "Waiting for commands");
                while (!commands_done_) {
                    message_ptr msg = messages_.pop ();
                    assert (msg);
                    this->process_command (fifo, *msg);
                    pool.return_to_pool (std::move (msg));
                }
            } catch (std::exception const & ex) {
                logging::log (logging::priority::error, "An error occurred: ", ex.what ());
            } catch (...) {
                logging::log (logging::priority::error, "Unknown error");
            }
            logging::log (logging::priority::info, "Exiting command thread");
        }

        // push_command
        // ~~~~~~~~~~~~
        void command_processor::push_command (message_ptr && cmd, recorder * const record_file) {
            if (record_file) {
                record_file->record (*cmd);
            }
            messages_.push (std::move (cmd));
        }

        // clear_queue
        // ~~~~~~~~~~~
        void command_processor::clear_queue () { messages_.clear (); }

        // scavenge
        // ~~~~~~~~
        void command_processor::scavenge () {
            logging::log (logging::priority::info, "Scavenging zombie commands");

            // TODO: make this time threshold user configurable
            constexpr auto delete_threshold = std::chrono::seconds (4 * 60 * 60);

            // After this function is complete, no partial messages older than 'earliest time' will
            // be in the partial command map (cmds_).
            auto const earliest_time = std::chrono::system_clock::now () - delete_threshold;

            // Remove all partial messages where the last piece of the message arrived before
            // earliest_time.
            // It's most likely that the sending process gave up/crashed/lost interest before
            // sending the complete message.
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

                    logging::log (logging::priority::info, "Deleted old partial message. Arrived ",
                                  time_str);
                    it = cmds_.erase (it);
                } else {
                    ++it;
                }
            }
        }

    } // namespace broker
} // namespace pstore

