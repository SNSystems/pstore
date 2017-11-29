//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===- tools/broker/main.cpp ----------------------------------------------===//
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

// Standard includes
#include <cstring>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <type_traits>
#include <vector>

// Platform includes
#ifndef _WIN32
#include <signal.h>
#endif

#include "broker/command.hpp"
#include "broker/gc.hpp"
#include "broker/globals.hpp"
#include "broker/quit.hpp"
#include "broker/read_loop.hpp"
#include "broker/recorder.hpp"
#include "broker/scavenger.hpp"
#include "pstore_broker_intf/fifo_path.hpp"
#include "pstore_broker_intf/message_type.hpp"
#include "pstore_broker_intf/unique_handle.hpp"
#include "pstore_support/logging.hpp"
#include "pstore_support/thread.hpp"
#include "pstore_support/utf.hpp"

#include "switches.hpp"

namespace {

    // create_thread
    // ~~~~~~~~~~~~~
    template <class Function, class... Args>
    auto create_thread (Function && f, Args &&... args)
        -> std::future<typename std::result_of<Function (Args...)>::type> {

        using return_type = typename std::result_of<Function (Args...)>::type;

        std::packaged_task<return_type (Args...)> task (f);
        auto fut = task.get_future ();

        std::thread thr (std::move (task), std::forward<Args> (args)...);
        thr.detach ();

        return fut;
    }

    /// Creates a weak_ptr from a shared_ptr. This can be done implicitly, but I want to make the
    /// conversion
    /// explicit.
    template <typename T>
    std::weak_ptr<T> make_weak (std::shared_ptr<T> const & p) {
        return {p};
    }

    void thread_init (std::string const & name) {
        pstore::threads::set_name (name.c_str ());
        pstore::logging::create_log_stream ("broker." + name);
    }

} // end anonymous namespace

#if defined(_WIN32)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    try {
        pstore::threads::set_name ("main");
        pstore::logging::create_log_stream ("broker.main");
        pstore::logging::log (pstore::logging::priority::notice, "broker starting");

        switches opt;
        std::tie (opt, exit_code) = get_switches (argc, argv);
        if (exit_code != EXIT_SUCCESS) {
            return exit_code;
        }

        // If we're recording the messages we receive, then create the file in which they will be
        // stored.
        std::shared_ptr<recorder> record_file;
        if (opt.record_path) {
            record_file = std::make_shared<recorder> (*opt.record_path);
        }

        // TODO: ensure that this is a singleton process?
        // auto const path = std::make_unique<fifo_path> ();
        pstore::logging::log (pstore::logging::priority::notice, "opening pipe");

        pstore::broker::fifo_path fifo{opt.pipe_path ? opt.pipe_path->c_str () : nullptr};

        std::vector<std::future<void>> futures;
        auto commands = std::make_shared<command_processor> (opt.num_read_threads);
        auto scav = std::make_shared<scavenger> (commands);
        commands->attach_scavenger (scav);

        pstore::logging::log (pstore::logging::priority::notice, "starting threads");
        std::thread quit =
            create_quit_thread (make_weak (commands), make_weak (scav), opt.num_read_threads);

        futures.push_back (create_thread ([&fifo, &commands]() {
            thread_init ("command");
            commands->thread_entry (fifo);
        }));

        futures.push_back (create_thread ([&scav]() {
            thread_init ("scavenger");
            scav->thread_entry ();
        }));

        futures.push_back (create_thread ([]() {
            thread_init ("gcwatch");
            broker::gc_process_watch_thread ();
        }));

        if (opt.playback_path) {
            player playback_file (*opt.playback_path);
            while (auto msg = playback_file.read ()) {
                commands->push_command (std::move (msg), record_file.get ());
            }
            shutdown (commands.get (), scav.get (), -1 /*signum*/, 0U /*num read threads*/);
        } else {
            for (auto ctr = 0U; ctr < opt.num_read_threads; ++ctr) {
                futures.push_back (create_thread ([&fifo, &record_file, &commands]() {
                    pstore::threads::set_name ("read");
                    pstore::logging::create_log_stream ("broker.read");
                    read_loop (fifo, record_file, commands);
                }));
            }
        }

        pstore::logging::log (pstore::logging::priority::notice, "waiting");
        for (auto & f : futures) {
            assert (f.valid ());
            f.get ();
        }
        notify_quit_thread ();
        quit.join ();
        pstore::logging::log (pstore::logging::priority::notice, "exiting");
    } catch (std::exception const & ex) {
        pstore::logging::log (pstore::logging::priority::error, "error: ", ex.what ());
        exit_code = EXIT_FAILURE;
    } catch (...) {
        pstore::logging::log (pstore::logging::priority::error, "unknown error");
        exit_code = EXIT_FAILURE;
    }
    return exit_code;
}
// eof: tools/broker/main.cpp
