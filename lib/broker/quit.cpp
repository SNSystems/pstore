//*              _ _    *
//*   __ _ _   _(_) |_  *
//*  / _` | | | | | __| *
//* | (_| | |_| | | |_  *
//*  \__, |\__,_|_|\__| *
//*     |_|             *
//===- lib/broker/quit.cpp ------------------------------------------------===//
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
/// \file quit.cpp

#include "pstore/broker/quit.hpp"

// Standard includes
#include <atomic>
#include <cassert>
#include <cerrno>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>

// Platform includes
#include <fcntl.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <signal.h>

#include "pstore/broker/command.hpp"
#include "pstore/broker/gc.hpp"
#include "pstore/broker/globals.hpp"
#include "pstore/broker/scavenger.hpp"
#include "pstore/broker_intf/fifo_path.hpp"
#include "pstore/broker_intf/message_type.hpp"
#include "pstore/broker_intf/status_client.hpp"
#include "pstore/core/make_unique.hpp"
#include "pstore/support/array_elements.hpp"
#include "pstore/support/logging.hpp"
#include "pstore/support/signal_cv.hpp"
#include "pstore/support/signal_helpers.hpp"
#include "pstore/support/thread.hpp"

namespace {

    // TODO: these are shared with the command processor and should be somewhere common.
    std::string const read_loop_quit_command{"_QUIT"};
    std::string const command_loop_quit_command{"_CQUIT"};

    // push
    // ~~~~
    /// Push a simple message onto the command queue.
    void push (pstore::broker::command_processor & cp, std::string const & message) {
        static std::atomic<std::uint32_t> mid{0};

        pstore::logging::log (pstore::logging::priority::info, "push command ",
                              pstore::logging::quoted (message.c_str ()));

        assert (message.length () <= pstore::broker::message_type::payload_chars);
        auto msg = std::make_unique<pstore::broker::message_type> (mid++, std::uint16_t{0},
                                                                   std::uint16_t{1}, message);
        cp.push_command (std::move (msg), nullptr);
    }

} // namespace

namespace pstore {
    namespace broker {

        // shutdown
        // ~~~~~~~~
        void shutdown (command_processor * const cp, scavenger * const scav, int signum,
                       unsigned num_read_threads, bool status_useinet) {
            // Set the global "done" flag unless we're already shutting down. The latter condition
            // happens if a "SUICIDE" command is received and the quit thread is woken in response.
            bool expected = false;
            if (done.compare_exchange_strong (expected, true)) {
                std::cerr << "pstore broker is exiting.\n";

                // Tell the gcwatcher thread to exit.
                broker::gc_sigint (signum);

                if (scav != nullptr) {
                    scav->shutdown ();
                }

                if (cp != nullptr) {
                    // Ask the read-loop threads to quit
                    for (auto ctr = 0U; ctr < num_read_threads; ++ctr) {
                        push (*cp, read_loop_quit_command);
                    }
                    // Finally ask the command loop thread to exit.
                    push (*cp, command_loop_quit_command);
                }

                auto socket = connect_to_status_server (status_useinet);
                char const json[] = "{\"quit\":true}\04";
                ::send (socket.get (), json, static_cast<int> (array_elements (json) - 1),
                        0 /*flags*/);
            }
        }

    } // namespace broker
} // namespace pstore

namespace {

    pstore::signal_cv quit_info;

    //***************
    //* quit thread *
    //***************
    void quit_thread (std::weak_ptr<pstore::broker::command_processor> cp,
                      std::weak_ptr<pstore::broker::scavenger> scav, unsigned num_read_threads,
                      bool status_useinet) {
        using namespace pstore;

        try {
            threads::set_name ("quit");
            logging::create_log_stream ("broker.quit");

            // Wait for to be told that we are in the process of shutting down. This
            // call will block until signal_handler() [below] is called by, for example,
            // the user typing Control-C.
            quit_info.wait ();

            logging::log (
                logging::priority::info,
                "Signal received. Will terminate after current command. Num=", quit_info.signal ());

            auto cp_sptr = cp.lock ();
            // If the command processor is alive, clear the queue.
            // TODO: prevent the queue from accepting commands other than the ones we're about
            // to send?
            if (cp_sptr) {
                cp_sptr->clear_queue ();
            }

            auto scav_sptr = scav.lock ();
            shutdown (cp_sptr.get (), scav_sptr.get (), quit_info.signal (), num_read_threads,
                      status_useinet);
        } catch (std::exception const & ex) {
            logging::log (logging::priority::error, "error:", ex.what ());
        } catch (...) {
            logging::log (logging::priority::error, "unknown exception");
        }
    }


    // signal_handler
    // ~~~~~~~~~~~~~~
    /// A signal handler entry point.
    void signal_handler (int sig) {
        pstore::broker::exit_code = sig;
        pstore::errno_saver saver;
        quit_info.notify (sig);
    }

} // end anonymous namespace


namespace pstore {
    namespace broker {

        // notify_quit_thread
        // ~~~~~~~~~~~~~~~~~~
        void notify_quit_thread () { quit_info.notify (-1); }

        // create_quit_thread
        // ~~~~~~~~~~~~~~~~~~
        std::thread create_quit_thread (std::weak_ptr<command_processor> cp,
                                        std::weak_ptr<scavenger> scav, unsigned num_read_threads,
                                        bool status_useinet) {
            std::thread quit (quit_thread, std::move (cp), std::move (scav), num_read_threads,
                              status_useinet);

            register_signal_handler (SIGINT, signal_handler);
            register_signal_handler (SIGTERM, signal_handler);
#ifdef _WIN32
            register_signal_handler (SIGBREAK, signal_handler); // Ctrl-Break sequence
#endif
            return quit;
        }

    } // namespace broker
} // namespace pstore

// eof: lib/broker/quit.cpp
