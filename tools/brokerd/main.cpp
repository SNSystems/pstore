//===- tools/brokerd/main.cpp ---------------------------------------------===//
//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
#include "pstore/brokerface/fifo_path.hpp"
#include "pstore/brokerface/message_type.hpp"
#include "pstore/config/config.hpp"
#include "pstore/http/server.hpp"
#include "pstore/http/server_status.hpp"
#include "pstore/os/descriptor.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/os/thread.hpp"
#include "pstore/os/wsa_startup.hpp"
#include "pstore/support/utf.hpp"
#include "pstore/broker/utils.hpp"

#include "switches.hpp"

using namespace std::string_literals;
using namespace pstore;

extern romfs::romfs fs;

#if defined(_WIN32)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    using priority = logger::priority;

    try {
        threads::set_name ("main");
        create_log_stream ("broker.main");
        log (priority::notice, "broker starting");

        switches opt;
        std::tie (opt, broker::exit_code) = get_switches (argc, argv);
        if (broker::exit_code != EXIT_SUCCESS) {
            return broker::exit_code;
        }

#ifdef _WIN32
        wsa_startup startup;
        if (!startup.started ()) {
            log (priority::error, "WSAStartup() failed");
            log (priority::info, "broker exited");
            return EXIT_FAILURE;
        }
#endif // _WIN32

        // If we're recording the messages we receive, then create the file in which they will be
        // stored.
        std::shared_ptr<broker::recorder> record_file;
        if (opt.record_path.has_value ()) {
            record_file = std::make_shared<broker::recorder> (*opt.record_path);
        }

        // TODO: ensure that this is a singleton process?
        log (priority::notice, "opening pipe");

        brokerface::fifo_path fifo{opt.pipe_path.has_value () ? opt.pipe_path->c_str () : nullptr};

        std::vector<std::future<void>> futures;
        std::thread quit;

        maybe<http::server_status> http_status = broker::get_http_server_status (opt.http_port);
        std::atomic<bool> uptime_done{false};

        log (priority::notice, "starting threads");
        {
            auto commands = std::make_shared<broker::command_processor> (
                opt.num_read_threads, &http_status, &uptime_done, opt.scavenge_time);
            auto scav = std::make_shared<broker::scavenger> (commands);
            commands->attach_scavenger (scav);

            quit = create_quit_thread (make_weak (commands), make_weak (scav), opt.num_read_threads,
                                       &http_status, &uptime_done);

            futures = create_worker_threads (commands, fifo, scav, &uptime_done);
            broker::create_http_worker_thread (&futures, &http_status, opt.announce_http_port, fs);

            if (opt.playback_path.has_value ()) {
                broker::player playback_file (*opt.playback_path);
                while (auto msg = playback_file.read ()) {
                    commands->push_command (std::move (msg), record_file.get ());
                }
                shutdown (commands.get (), scav.get (), -1 /*signum*/, 0U /*num read threads*/,
                          &http_status, &uptime_done);
            } else {
                for (auto ctr = 0U; ctr < opt.num_read_threads; ++ctr) {
                    futures.push_back (
                        broker::create_thread ([ctr, &fifo, &record_file, commands] () {
                            auto const name = "read"s + std::to_string (ctr);
                            threads::set_name (name.c_str ());
                            create_log_stream ("broker." + name);
                            read_loop (fifo, record_file, commands);
                        }));
                }
            }
        }

        log (priority::notice, "waiting");
        for (auto & f : futures) {
            PSTORE_ASSERT (f.valid ());
            f.get ();
        }
        log (priority::notice, "worker threads done: stopping quit thread");
        broker::notify_quit_thread ();
        quit.join ();
        log (priority::notice, "exiting");
    } catch (std::exception const & ex) {
        log (priority::error, "error: ", ex.what ());
        broker::exit_code = EXIT_FAILURE;
    } catch (...) {
        log (priority::error, "unknown error");
        broker::exit_code = EXIT_FAILURE;
    }
    return broker::exit_code;
}
