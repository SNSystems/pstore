//*                                                _  *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| | *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` | *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| | *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_| *
//*                                                   *
//===- include/pstore/broker/command.hpp ----------------------------------===//
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
/// \file command.hpp

#ifndef PSTORE_BROKER_COMMAND_HPP
#define PSTORE_BROKER_COMMAND_HPP

#include <array>
#include <atomic>
#include <cstring>
#include <mutex>
#include <tuple>

// pstore includes
#include "pstore/broker/message_queue.hpp"
#include "pstore/broker/parser.hpp"
#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/broker_intf/message_type.hpp"
#include "pstore/broker_intf/signal_cv.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/pubsub.hpp"

namespace pstore {
    namespace httpd {
        class server_status;
    } // end namespace httpd

    namespace broker {

        class fifo_path;
        class recorder;
        class scavenger;

        /// A class which is responsible for managing the command queue and which provides the
        /// thread_entry() function for pulling commands from the queue and executing them.
        class command_processor {
        public:
            /// \param num_read_threads  The number of threads listening to the command pipe.
            /// \param http_status  A pointer to an object which can be used to tell the http server
            ///   to exit.
            /// \param uptime_done  A pointer to a bool which, when set, will tell the
            ///   uptime thread to exit.
            /// \param scavenge_threshold  The time for which messages are
            ///   allowed to wait in the message queue before the scavenger will delete them.
            command_processor (unsigned num_read_threads,
                               gsl::not_null<httpd::server_status *> http_status,
                               gsl::not_null<std::atomic<bool> *> uptime_done,
                               std::chrono::seconds scavenge_threshold);
            // No copying or assignment.
            command_processor (command_processor const &) = delete;
            command_processor (command_processor &&) noexcept = delete;

            virtual ~command_processor () noexcept = default;

            // No copying or assignment.
            command_processor & operator= (command_processor const &) = delete;
            command_processor & operator= (command_processor &&) noexcept = delete;

            void thread_entry (fifo_path const & fifo);

            void attach_scavenger (std::shared_ptr<scavenger> & scav) { scavenger_.set (scav); }

            /// Pushes a command onto the end of the command queue. The command is recorded if
            /// `record_file` is not null.
            /// \param cmd  The message to which this parameter points is moved to the end of the
            /// command queue.
            /// \param record_file  If not null, this object is used to record the command.
            void push_command (message_ptr && cmd, recorder * record_file);
            void clear_queue ();

            void scavenge ();

            /// \note Public for unit testing.
            void process_command (fifo_path const & fifo, message_type const & msg);

        private:
            template <typename T>
            class atomic_weak_ptr {
            public:
                std::shared_ptr<T> get () {
                    std::lock_guard<decltype (mut_)> const lock{mut_};
                    return ptr_.lock ();
                }

                void set (std::shared_ptr<T> const & t) {
                    std::lock_guard<decltype (mut_)> const lock{mut_};
                    ptr_ = t;
                }
                void set (std::weak_ptr<T> & t) {
                    std::lock_guard<decltype (mut_)> const lock{mut_};
                    ptr_ = t;
                }

            private:
                std::mutex mut_;
                std::weak_ptr<T> ptr_;
            };

            std::atomic<bool> commands_done_{false};

            /// A pointer to an object which can be used to tell the http server to exit.
            gsl::not_null<httpd::server_status *> http_status_;
            /// A pointer to a bool which, when set, will tell the uptime thread to exit.
            gsl::not_null<std::atomic<bool> *> uptime_done_;
            /// The time for which messages are allowed to wait in the message queue before the
            /// scavenger will delete them.
            std::chrono::seconds delete_threshold_;

            atomic_weak_ptr<scavenger> scavenger_;

            message_queue<message_ptr> messages_;

            std::mutex cmds_mut_;
            partial_cmds cmds_;

            /// The number of read threads running. At shutdown time this is used to instruct each
            /// of them to exit safely.
            unsigned const num_read_threads_;

            /// The nuber of commit ("GC") commands processed.
            unsigned commits_ = 0;

            auto parse (message_type const & msg) -> std::unique_ptr<broker_command>;

            using handler = std::function<void (command_processor *, fifo_path const & fifo,
                                                broker_command const &)>;
            using command_entry = std::tuple<gsl::czstring, handler>;
            /// A predicate function used to sort and search a container of command_entry instances.
            static bool command_entry_compare (command_entry const & a, command_entry const & b) {
                return std::strcmp (std::get<0> (a), std::get<0> (b)) < 0;
            }

            static std::array<command_entry, 6> const commands_;

            ///@{
            /// Functions responsible for processing each of the commands to which the broker will
            /// respond in some way.
            /// \note Virtual to enable unit testing.

            /// Initiates the shut-down of the broker process.
            virtual void suicide (fifo_path const & fifo, broker_command const & c);

            /// Shuts down a single pipe-reader thread if the 'done' flag has been set by a call to
            /// ::shutdown().
            virtual void quit (fifo_path const & fifo, broker_command const & c);

            /// Calling this function causes the command_processor thread to exit.
            virtual void cquit (fifo_path const & fifo, broker_command const & c);

            /// Starts the garbage collection for a path (specified in the command path) if not
            /// already running.
            virtual void gc (fifo_path const & fifo, broker_command const & c);

            /// Echoes the command path text to stdout.
            virtual void echo (fifo_path const & fifo, broker_command const & c);

            /// A simple no-op command.
            virtual void nop (fifo_path const & fifo, broker_command const & c);
            ///@}

            /// Called to report the receipt of an unknown command verb.
            /// \note Virtual to enable unit testing.
            virtual void unknown (broker_command const & c) const;

            ///@{
            /// \note Virtual to enable unit testing.
            virtual void log (broker_command const & c) const;
            virtual void log (gsl::czstring str) const;
            ///@}
        };

        extern descriptor_condition_variable commits_cv;
        extern channel<descriptor_condition_variable> commits_channel;

    } // end namespace broker
} // end namespace pstore

#endif // PSTORE_BROKER_COMMAND_HPP
