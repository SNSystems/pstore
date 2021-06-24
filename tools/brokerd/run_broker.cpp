//===- tools/brokerd/run_broker.cpp ---------------------------------------===//
//*                     _               _              *
//*  _ __ _   _ _ __   | |__  _ __ ___ | | _____ _ __  *
//* | '__| | | | '_ \  | '_ \| '__/ _ \| |/ / _ \ '__| *
//* | |  | |_| | | | | | |_) | | | (_) |   <  __/ |    *
//* |_|   \__,_|_| |_| |_.__/|_|  \___/|_|\_\___|_|    *
//*                                                    *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "run_broker.hpp"

// Standard includes
#include <cstring>
#include <exception>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <signal.h>
#include <string>
#include <type_traits>
#include <vector>

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

#include "switches.hpp"

using namespace std::string_literals;
using namespace pstore;

extern romfs::romfs fs;

namespace {
    // create thread
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

    // thread init
    // ~~~~~~~~~~~
    void thread_init (std::string const & name) {
        threads::set_name (name.c_str ());
        create_log_stream ("broker." + name);
    }

    // create http worker thread
    // ~~~~~~~~~~~~~~~~~~~~~~~~~
    void create_http_worker_thread (gsl::not_null<std::vector<std::future<void>> *> const futures,
                                    gsl::not_null<maybe<http::server_status> *> http_status,
                                    bool announce_port) {
        if (!http_status->has_value ()) {
            return;
        }
        futures->push_back (create_thread (
            [announce_port] (maybe<http::server_status> * const status) {
                thread_init ("http");
                PSTORE_ASSERT (status->has_value ());

                http::channel_container channels{
                    {"commits",
                     http::channel_container_entry{&broker::commits_channel, &broker::commits_cv}},
                    {"uptime",
                     http::channel_container_entry{&broker::uptime_channel, &broker::uptime_cv}},
                };

                http::server (fs, &status->value (), channels,
                              [announce_port] (in_port_t const port) {
                                  if (announce_port) {
                                      std::lock_guard<decltype (broker::iomut)> lock{broker::iomut};
                                      std::cout << "HTTP listening on port " << port << std::endl;
                                  }
                              });
            },
            http_status.get ()));
    }

    // create worker threads
    // ~~~~~~~~~~~~~~~~~~~~~
    std::vector<std::future<void>> create_worker_threads (
        std::shared_ptr<broker::command_processor> const & commands, brokerface::fifo_path & fifo,
        std::shared_ptr<broker::scavenger> const & scav, std::atomic<bool> * const uptime_done) {

        std::vector<std::future<void>> futures;

        futures.push_back (create_thread ([&fifo, commands] () {
            thread_init ("command");
            commands->thread_entry (fifo);
        }));

        futures.push_back (create_thread ([scav] () {
            thread_init ("scavenger");
            scav->thread_entry ();
        }));

        futures.push_back (create_thread ([] () {
            thread_init ("gcwatch");
            broker::gc_process_watch_thread ();
        }));

        futures.push_back (create_thread (
            [] (std::atomic<bool> * const done) {
                thread_init ("uptime");
                broker::uptime (done);
            },
            uptime_done));

        return futures;
    }

    // make weak
    // ~~~~~~~~~
    /// Creates a weak_ptr from a shared_ptr. This can be done implicitly, but I want to make
    /// the conversion explicit.
    template <typename T>
    std::weak_ptr<T> make_weak (std::shared_ptr<T> const & p) {
        return {p};
    }

    // get http server status
    // ~~~~~~~~~~~~~~~~~~~~~~
    /// Create an HTTP server_status object which reflects the user's choice of port.
    decltype (auto) get_http_server_status (maybe<in_port_t> const & port) {
        if (port.has_value ()) {
            return maybe<http::server_status>{in_place, *port};
        }
        return nothing<http::server_status> ();
    }
} // namespace

int run_broker (switches const & opt) {
    using priority = logger::priority;

    threads::set_name ("main");
    create_log_stream ("broker.main");
    log (priority::notice, "broker starting");

#ifdef _WIN32
    wsa_startup startup;
    if (!startup.started ()) {
        throw std::runtime_error ("WSAStartup() failed, broker exited");
    }
#endif // _WIN32

    // If we're recording the messages we receive, then create the file in which they
    // will be stored.
    std::shared_ptr<broker::recorder> record_file;
    if (opt.record_path.has_value ()) {
        record_file = std::make_shared<broker::recorder> (*opt.record_path);
    }

    // TODO: ensure that this is a singleton process?
    log (priority::notice, "opening pipe");

    brokerface::fifo_path fifo{opt.pipe_path.has_value () ? opt.pipe_path->c_str () : nullptr};

    std::vector<std::future<void>> futures;
    std::thread quit;

    maybe<http::server_status> http_status = get_http_server_status (opt.http_port);
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
        create_http_worker_thread (&futures, &http_status, opt.announce_http_port);

        if (opt.playback_path.has_value ()) {
            broker::player playback_file (*opt.playback_path);
            while (auto msg = playback_file.read ()) {
                commands->push_command (std::move (msg), record_file.get ());
            }
            shutdown (commands.get (), scav.get (), -1 /*signum*/, 0U /*num read threads*/,
                      &http_status, &uptime_done);
        } else {
            for (auto ctr = 0U; ctr < opt.num_read_threads; ++ctr) {
                futures.push_back (create_thread ([ctr, &fifo, &record_file, commands] () {
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
    return broker::exit_code;
}
