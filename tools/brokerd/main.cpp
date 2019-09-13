//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===- tools/brokerd/main.cpp ---------------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
#include <string>
#include <type_traits>
#include <vector>

// Platform includes
#ifndef _WIN32
#    include <signal.h>
#endif

#include "pstore/broker/command.hpp"
#include "pstore/broker/gc.hpp"
#include "pstore/broker/globals.hpp"
#include "pstore/broker/quit.hpp"
#include "pstore/broker/read_loop.hpp"
#include "pstore/broker/recorder.hpp"
#include "pstore/broker/scavenger.hpp"
#include "pstore/broker/uptime.hpp"
#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/broker_intf/fifo_path.hpp"
#include "pstore/broker_intf/message_type.hpp"
#include "pstore/broker_intf/wsa_startup.hpp"
#include "pstore/config/config.hpp"
#include "pstore/http/server.hpp"
#include "pstore/http/server_status.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/os/thread.hpp"
#include "pstore/support/to_string.hpp"
#include "pstore/support/utf.hpp"

#include "switches.hpp"

extern pstore::romfs::romfs fs;

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
    /// conversion explicit.
    template <typename T>
    std::weak_ptr<T> make_weak (std::shared_ptr<T> const & p) {
        return {p};
    }

    void thread_init (std::string const & name) {
        pstore::threads::set_name (name.c_str ());
        pstore::logging::create_log_stream ("broker." + name);
    }


    std::vector<std::future<void>>
    create_worker_threads (std::shared_ptr<pstore::broker::command_processor> const & commands,
                           pstore::broker::fifo_path & fifo,
                           std::shared_ptr<pstore::broker::scavenger> const & scav,
                           std::unique_ptr<pstore::httpd::server_status> const & http_status,
                           std::atomic<bool> * const uptime_done) {

        using namespace pstore;
        std::vector<std::future<void>> futures;

        futures.push_back (create_thread ([&fifo, commands]() {
            thread_init ("command");
            commands->thread_entry (fifo);
        }));

        futures.push_back (create_thread ([scav]() {
            thread_init ("scavenger");
            scav->thread_entry ();
        }));

        futures.push_back (create_thread ([]() {
            thread_init ("gcwatch");
            broker::gc_process_watch_thread ();
        }));

        futures.push_back (create_thread (
            [](httpd::server_status * const status) {
                thread_init ("http");
                pstore::httpd::channel_container channels{
                    {"commits",
                     pstore::httpd::channel_container_entry{&pstore::broker::commits_channel,
                                                            &pstore::broker::commits_cv}},
                    {"uptime",
                     pstore::httpd::channel_container_entry{&pstore::broker::uptime_channel,
                                                            &pstore::broker::uptime_cv}},
                };
                httpd::server (fs, status, channels);
            },
            http_status.get ()));

        futures.push_back (create_thread (
            [](std::atomic<bool> * const done) {
                thread_init ("uptime");
                broker::uptime (done);
            },
            uptime_done));

        return futures;
    }

} // end anonymous namespace

#if defined(_WIN32)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    using namespace pstore;

    try {
        threads::set_name ("main");
        logging::create_log_stream ("broker.main");
        logging::log (logging::priority::notice, "broker starting");

        switches opt;
        std::tie (opt, broker::exit_code) = get_switches (argc, argv);
        if (broker::exit_code != EXIT_SUCCESS) {
            return broker::exit_code;
        }

#ifdef _WIN32
        pstore::wsa_startup startup;
        if (!startup.started ()) {
            log (logging::priority::error, "WSAStartup() failed");
            log (pstore::logging::priority::info, "broker exited");
            return EXIT_FAILURE;
        }
#endif // _WIN32

        // If we're recording the messages we receive, then create the file in which they will be
        // stored.
        std::shared_ptr<broker::recorder> record_file;
        if (opt.record_path) {
            record_file = std::make_shared<broker::recorder> (*opt.record_path);
        }

        // TODO: ensure that this is a singleton process?
        // auto const path = std::make_unique<fifo_path> ();
        logging::log (logging::priority::notice, "opening pipe");

        broker::fifo_path fifo{opt.pipe_path ? opt.pipe_path->c_str () : nullptr};

        std::vector<std::future<void>> futures;
        std::thread quit;

        constexpr in_port_t http_port = 8080;
        auto http_status = std::unique_ptr<pstore::httpd::server_status> (
            new pstore::httpd::server_status (http_port));
        std::atomic<bool> uptime_done{false};

        {
            auto commands = std::make_shared<broker::command_processor> (
                opt.num_read_threads, http_status.get (), &uptime_done);
            auto scav = std::make_shared<broker::scavenger> (commands);
            commands->attach_scavenger (scav);


            logging::log (logging::priority::notice, "starting threads");
            quit = create_quit_thread (make_weak (commands), make_weak (scav), opt.num_read_threads,
                                       http_status.get (), &uptime_done);

            futures = create_worker_threads (commands, fifo, scav, http_status, &uptime_done);

            if (opt.playback_path) {
                broker::player playback_file (*opt.playback_path);
                while (auto msg = playback_file.read ()) {
                    commands->push_command (std::move (msg), record_file.get ());
                }
                shutdown (commands.get (), scav.get (), -1 /*signum*/, 0U /*num read threads*/,
                          http_status.get (), &uptime_done);
            } else {
                for (auto ctr = 0U; ctr < opt.num_read_threads; ++ctr) {
                    futures.push_back (create_thread ([ctr, &fifo, &record_file, commands]() {
                        auto const name = std::string{"read"} + pstore::to_string (ctr);
                        threads::set_name (name.c_str ());
                        logging::create_log_stream ("broker." + name);
                        read_loop (fifo, record_file, commands);
                    }));
                }
            }
        }

        logging::log (logging::priority::notice, "waiting");
        for (auto & f : futures) {
            assert (f.valid ());
            f.get ();
        }
        logging::log (logging::priority::notice, "worker threads done: stopping quit thread");
        broker::notify_quit_thread ();
        quit.join ();
        logging::log (logging::priority::notice, "exiting");
    } catch (std::exception const & ex) {
        logging::log (logging::priority::error, "error: ", ex.what ());
        broker::exit_code = EXIT_FAILURE;
    } catch (...) {
        logging::log (logging::priority::error, "unknown error");
        broker::exit_code = EXIT_FAILURE;
    }
    return broker::exit_code;
}
