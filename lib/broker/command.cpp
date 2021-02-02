//*                                                _  *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| *
//*                                                   *
//===- lib/broker/command.cpp ---------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file command.cpp

#include "pstore/broker/command.hpp"

#include <iomanip>
#include <sstream>

#include "pstore/broker/gc.hpp"
#include "pstore/broker/globals.hpp"
#include "pstore/broker/internal_commands.hpp"
#include "pstore/broker/message_pool.hpp"
#include "pstore/broker/quit.hpp"
#include "pstore/broker/recorder.hpp"
#include "pstore/brokerface/writer.hpp"
#include "pstore/json/utility.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/os/time.hpp"

namespace {

    /// Converts a time point to a string.
    ///
    /// \param time  The time point whose string equivalent is to be returned.
    /// \param buffer  A buffer into which the string is written.
    /// \returns  A null terminated string. Will always be a pointer to the buffer base address.
    auto time_to_string (std::chrono::system_clock::time_point const time,
                         std::array<char, 100> * const buffer) -> pstore::gsl::zstring {
        std::tm const tm = pstore::gm_time (std::chrono::system_clock::to_time_t (time));
        // strftime() returns 0 if the result doesn't fit in the provided buffer;
        // the contents are undefined.
        if (std::strftime (buffer->data (), buffer->size (), "%FT%TZ", &tm) == 0) {
            std::strncpy (buffer->data (), "(unknown)", buffer->size ());
        }
        // Guarantee that the string is null terminated.
        PSTORE_ASSERT (buffer->size () > 0);
        (*buffer)[buffer->size () - 1] = '\0';
        return buffer->data ();
    }

    using priority = pstore::logger::priority;

} // end anonymous namespace

namespace pstore {
    namespace broker {

        descriptor_condition_variable commits_cv;
        brokerface::channel<descriptor_condition_variable> commits_channel{&commits_cv};

        // ctor
        // ~~~~
        command_processor::command_processor (
            unsigned const num_read_threads,
            gsl::not_null<maybe<http::server_status> *> const http_status,
            gsl::not_null<std::atomic<bool> *> const uptime_done,
            std::chrono::seconds const delete_threshold)
                : http_status_{http_status}
                , uptime_done_{uptime_done}
                , delete_threshold_{delete_threshold}
                , num_read_threads_{num_read_threads} {
            PSTORE_ASSERT (std::is_sorted (std::begin (commands_), std::end (commands_),
                                           command_entry_compare));
        }

        // suicide
        // ~~~~~~~
        void command_processor::suicide (brokerface::fifo_path const &, broker_command const &) {
            std::shared_ptr<scavenger> const scav = scavenger_.get ();
            shutdown (this, scav.get (), sig_self_quit, num_read_threads_, http_status_,
                      uptime_done_);
        }

        // quit
        // ~~~~
        void command_processor::quit (brokerface::fifo_path const & fifo, broker_command const &) {
            // Shut down a single pipe-reader thread if the 'done' flag has been set by a call to
            // shutdown().
            if (!done) {
                this->log ("_QUIT ignored: not shutting down");
            } else {
                this->log ("waking one reader thread");

                // Post a message back to one of the read-loop threads. Since 'done' is now true, it
                // will exit as soon as it is woken up by the presence of the new data (the content
                // of the message doesn't matter at all).
                brokerface::writer wr (fifo);
                wr.write (brokerface::message_type{}, true /*issue error on timeout*/);
            }
        }

        // cquit
        // ~~~~~
        void command_processor::cquit (brokerface::fifo_path const &, broker_command const &) {
            commands_done_ = true;
        }

        // gc
        // ~~
        void command_processor::gc (brokerface::fifo_path const &, broker_command const & c) {
            start_vacuum (c.path);

            ++commits_;
            commits_channel.publish ([this] () {
                std::ostringstream os;
                os << "{ \"commits\": " << commits_ << " }";
                std::string const & str = os.str ();
                PSTORE_ASSERT (json::is_valid (str));
                return str;
            });
        }

        // echo
        // ~~~~
        void command_processor::echo (brokerface::fifo_path const &, broker_command const & c) {
            std::lock_guard<decltype (iomut)> lock{iomut};
            std::printf ("ECHO:%s\n", c.path.c_str ());
        }

        // nop
        // ~~~
        void command_processor::nop (brokerface::fifo_path const &, broker_command const &) {}

        // unknown
        // ~~~~~~~
        void command_processor::unknown (broker_command const & c) const {
            pstore::log (priority::error, "unknown verb:", c.verb);
        }

        // log
        // ~~~
        void command_processor::log (broker_command const & c) const {
            static constexpr char const verb[] = "verb:";
            static constexpr auto verb_length = array_elements (verb) - 1;
            static constexpr char const path[] = " path:";
            static constexpr auto path_length = array_elements (path) - 1;
            static constexpr auto max_path_length = std::size_t{32};
            static constexpr char const ellipsis[] = "...";
            static constexpr auto ellipsis_length = array_elements (ellipsis) - 1;

            std::string message;
            message.reserve (verb_length + c.verb.length () + path_length + max_path_length +
                             ellipsis_length);

            message = verb + c.verb + path;
            message += (c.path.length () < max_path_length)
                           ? c.path
                           : (c.path.substr (0, max_path_length) + ellipsis);
            pstore::log (priority::info, message.c_str ());
        }

        void command_processor::log (gsl::czstring const str) const {
            pstore::log (priority::info, str);
        }

        std::array<command_processor::command_entry, 6> const command_processor::commands_{{
            command_processor::command_entry ("ECHO", &command_processor::echo),
            command_processor::command_entry ("GC", &command_processor::gc),
            command_processor::command_entry ("NOP", &command_processor::nop),
            command_processor::command_entry (
                "SUICIDE", &command_processor::suicide), // initiate the broker shutdown.

            // Internal commands.
            command_processor::command_entry (
                command_loop_quit_command,
                &command_processor::cquit), // exit this command processor thread.
            command_processor::command_entry (
                read_loop_quit_command,
                &command_processor::quit), //  shut down a single pipe-reader thread.
        }};

        // process_command
        // ~~~~~~~~~~~~~~~
        void command_processor::process_command (brokerface::fifo_path const & fifo,
                                                 brokerface::message_type const & msg) {
            auto const command = this->parse (msg);
            if (broker_command const * const c = command.get ()) {
                this->log (*c);
                auto const pos = std::lower_bound (std::begin (commands_), std::end (commands_),
                                                   command_entry (c->verb.c_str (), nullptr),
                                                   command_entry_compare);
                if (pos != std::end (commands_) && c->verb == std::get<gsl::czstring> (*pos)) {
                    std::get<handler> (*pos) (this, fifo, *c);
                } else {
                    this->unknown (*c);
                }
            }
        }

        // parse
        // ~~~~~
        auto command_processor::parse (brokerface::message_type const & msg)
            -> std::unique_ptr<broker_command> {
            std::lock_guard<decltype (cmds_mut_)> const lock{cmds_mut_};
            return ::pstore::broker::parse (msg, cmds_);
        }

        // thread_entry
        // ~~~~~~~~~~~~
        void command_processor::thread_entry (brokerface::fifo_path const & fifo) {
            try {
                pstore::log (priority::info, "Waiting for commands");
                while (!commands_done_) {
                    brokerface::message_ptr msg = messages_.pop ();
                    PSTORE_ASSERT (msg);
                    this->process_command (fifo, *msg);
                    pool.return_to_pool (std::move (msg));
                }
            } catch (std::exception const & ex) {
                pstore::log (priority::error, "An error occurred: ", ex.what ());
            } catch (...) {
                pstore::log (priority::error, "Unknown error");
            }
            pstore::log (priority::info, "Exiting command thread");
        }

        // push_command
        // ~~~~~~~~~~~~
        void command_processor::push_command (brokerface::message_ptr && cmd,
                                              recorder * const record_file) {
            if (record_file != nullptr) {
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
            pstore::log (priority::info, "Scavenging zombie commands");

            // After this function is complete, no partial messages older than 'earliest time' will
            // be in the partial command map (cmds_).
            auto const earliest_time = std::chrono::system_clock::now () - delete_threshold_;

            // Remove all partial messages where the last piece of the message arrived before
            // earliest_time. It's most likely that the sending process gave up/crashed/lost
            // interest before sending the complete message.
            std::lock_guard<decltype (cmds_mut_)> const lock{cmds_mut_};
            auto const end = std::end (cmds_);
            auto it = std::begin (cmds_);
            while (it != end) {
                auto const arrival_time = it->second.arrive_time_;
                if (arrival_time < earliest_time) {
                    std::array<char, 100> buffer;
                    pstore::log (priority::info, "Deleted old partial message. Arrived ",
                                 time_to_string (arrival_time, &buffer));
                    it = cmds_.erase (it);
                } else {
                    ++it;
                }
            }
        }

    } // end namespace broker
} // end namespace pstore
