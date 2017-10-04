//*              _ _    *
//*   __ _ _   _(_) |_  *
//*  / _` | | | | | __| *
//* | (_| | |_| | | |_  *
//*  \__, |\__,_|_|\__| *
//*     |_|             *
//===- lib/broker/quit.cpp ------------------------------------------------===//
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
/// \file quit.cpp

#include "broker/quit.hpp"

// Standard includes
#include <atomic>
#include <cassert>
#include <cerrno>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>

// Platform includes
#include <fcntl.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <signal.h>

// pstore includes
#include "pstore_broker_intf/fifo_path.hpp"
#include "pstore_broker_intf/message_type.hpp"
#include "pstore_support/logging.hpp"
#include "pstore_support/signal_cv.hpp"
#include "pstore_support/signal_helpers.hpp"
#include "pstore_support/thread.hpp"
#include "pstore/make_unique.hpp"

// Local includes
#include "broker/command.hpp"
#include "broker/gc.hpp"
#include "broker/globals.hpp"
#include "broker/scavenger.hpp"

namespace {
    // TODO: these are shared with the command processor and should be somewhere common.
    std::string const read_loop_quit_command{"_QUIT"};
    std::string const command_loop_quit_command{"_CQUIT"};

    // push
    // ~~~~
    /// Push a simple message onto the command queue.
    void push (command_processor & cp, std::string const & message) {
        static std::atomic<std::uint32_t> mid{0};

        pstore::logging::log (pstore::logging::priority::info, "push command \"", message, '\"');

        assert (message.length () <= pstore::broker::message_type::payload_chars);
        auto msg = std::make_unique<pstore::broker::message_type> (mid++, std::uint16_t{0},
                                                                   std::uint16_t{1}, message);
        cp.push_command (std::move (msg), nullptr);
    }
}

// shutdown
// ~~~~~~~~
void shutdown (command_processor * const cp, scavenger * const scav, int signum,
               unsigned num_read_threads) {
    // Set the global "done" flag unless we're already shutting down. The latter condition happens
    // if a "SUICIDE" command is received and the quit thread is woken in response.
    bool expected = false;
    if (done.compare_exchange_strong (expected, true)) {
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
    }
}


namespace {

    pstore::signal_cv quit_info;

    //***************
    //* quit thread *
    //***************
    void quit_thread (std::weak_ptr<command_processor> const & cp,
                      std::weak_ptr<scavenger> const & scav, unsigned num_read_threads) {
        try {
            pstore::threads::set_name ("quit");
            pstore::logging::create_log_stream ("broker.quit");

            // Wait for to be told that we are in the process of shutting down. This
            // call will block until signal_handler() [below] is called by, for example,
            // the user typing Control-C.
            quit_info.wait ();

            pstore::logging::log (pstore::logging::priority::info, "got signal ",
                                  quit_info.signal (),
                                  ", will terminate after current command ends");

            auto cp_sptr = cp.lock ();
            // If the command processor is alive, clear the queue.
            // TODO: prevent the queue from accepting commands other than the ones we're about
            // to send?
            if (cp_sptr) {
                cp_sptr->clear_queue ();
            }
            auto scav_sptr = scav.lock ();
            shutdown (cp_sptr.get (), scav_sptr.get (), quit_info.signal (), num_read_threads);
        } catch (std::exception const & ex) {
            pstore::logging::log (pstore::logging::priority::error, "error:", ex.what ());
        } catch (...) {
            pstore::logging::log (pstore::logging::priority::error, "unknown exception");
        }
    }


    // signal_handler
    // ~~~~~~~~~~~~~~
    /// A signal handler entry point.
    void signal_handler (int sig) {
        pstore::errno_saver saver;
        quit_info.notify (sig);
    }

} // (anonymous namespace)


// notify_quit_thread
// ~~~~~~~~~~~~~~~~~~
void notify_quit_thread () {
    quit_info.notify (-1);
}

// create_quit_thread
// ~~~~~~~~~~~~~~~~~~
std::thread create_quit_thread (std::weak_ptr<command_processor> const & cp,
                                std::weak_ptr<scavenger> const & scav, unsigned num_read_threads) {
    std::thread quit (quit_thread, std::ref (cp), std::ref (scav), num_read_threads);
    pstore::register_signal_handler (SIGINT, signal_handler);
    pstore::register_signal_handler (SIGTERM, signal_handler);
#ifdef _WIN32
    pstore::register_signal_handler (SIGBREAK, signal_handler); // Ctrl-Break sequence
#endif
    return quit;
}

// eof: lib/broker/quit.cpp
