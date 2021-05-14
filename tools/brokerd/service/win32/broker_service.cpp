//===- tools/brokerd/service/win32/broker_service.cpp ---------------------===//
//*  _               _                                   _           *
//* | |__  _ __ ___ | | _____ _ __   ___  ___ _ ____   _(_) ___ ___  *
//* | '_ \| '__/ _ \| |/ / _ \ '__| / __|/ _ \ '__\ \ / / |/ __/ _ \ *
//* | |_) | | | (_) |   <  __/ |    \__ \  __/ |   \ V /| | (_|  __/ *
//* |_.__/|_|  \___/|_|\_\___|_|    |___/\___|_|    \_/ |_|\___\___| *
//*                                                                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <thread>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "broker_service.hpp"

#include <Dbghelp.h>

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

    // get http server status
    // ~~~~~~~~~~~~~~~~~~~~~~
    /// Create an HTTP server_status object which reflects the user's choice of port.
    decltype (auto) get_http_server_status (maybe<in_port_t> const & port) {
        if (port.has_value ()) {
            return maybe<http::server_status>{in_place, *port};
        }
        return nothing<http::server_status> ();
    }

    // make weak
    // ~~~~~~~~~
    /// Creates a weak_ptr from a shared_ptr. This can be done implicitly, but I want to make the
    /// conversion explicit.
    template <typename T>
    std::weak_ptr<T> make_weak (std::shared_ptr<T> const & p) {
        return {p};
    }

} // end anonymous namespace

// (ctor)
// ~~~~~~
broker_service::broker_service (TCHAR const * service_name, bool can_stop, bool can_shutdown,
                                bool can_pause_continue)
        : service_base (service_name, can_stop, can_shutdown, can_pause_continue) {}

// (dtor)
// ~~~~~~
broker_service::~broker_service () {

}

// start_handler
// ~~~~~~~~~~~~~
/// \brief Executed when a 'start' command is sent to the service by the SCM or when the operating
/// system starts (for a service that starts automatically).
/// \note start_handler() must return to the operating system after the service's operation has
/// begun: it must not block.

void broker_service::start_handler (DWORD argc, TCHAR * argv[]) {
    // Log a service start message to the Application log.
    this->write_event_log_entry ("broker service starting", event_type::information);

    std::pair<switches, int> opts = get_switches (argc, argv);

    create_thread ([this, opts] { this->worker (opts.first); });
}


// worker
// ~~~~~~
void broker_service::worker (switches opt) {
    this->write_event_log_entry ("Worker started", event_type::information);

    try {
        wsa_startup startup;
        if (!startup.started ()) {
            this->write_event_log_entry ("WSAStartup() failed, broker exited", event_type::error);
            this->set_service_status (SERVICE_STOPPED, EXIT_FAILURE);
            return;
        }

        this->write_event_log_entry ("opening pipe", event_type::information);
        brokerface::fifo_path fifo{nullptr};

        std::vector<std::future<void>> futures;
        std::thread quit;
        maybe<http::server_status> http_status = get_http_server_status (maybe<in_port_t>(8080));

        std::atomic<bool> uptime_done{false};

        std::shared_ptr<broker::recorder> record_file;

        auto commands = std::make_shared<broker::command_processor> (
            opt.num_read_threads, &http_status, &uptime_done, opt.scavenge_time);
        auto scav = std::make_shared<broker::scavenger> (commands);
        commands->attach_scavenger (scav);

        this->write_event_log_entry ("starting threads", event_type::information);

        quit = create_quit_thread (make_weak (commands), make_weak (scav), opt.num_read_threads,
                                   &http_status, &uptime_done);

        futures = create_worker_threads (commands, fifo, scav, &uptime_done);
        create_http_worker_thread (&futures, &http_status, opt.num_read_threads);

        for (auto ctr = 0U; ctr < opt.num_read_threads; ++ctr) {
            futures.push_back (create_thread ([ctr, &fifo, &record_file, commands] () {
                auto const name = "read"s + std::to_string (ctr);
                threads::set_name (name.c_str ());
                create_log_stream ("broker." + name);
                read_loop (fifo, record_file, commands);
            }));
        }

        this->write_event_log_entry ("waiting on threads", event_type::information);
        for (auto & f : futures) {
            PSTORE_ASSERT (f.valid ());
            f.get ();
        }

        this->write_event_log_entry ("worker threads done, stopping quit thread", event_type::information);
        broker::notify_quit_thread ();
        quit.join ();

    } catch (std::exception const & ex) {
        this->write_event_log_entry (std::string("error: ", ex.what()).c_str(), event_type::error);
        this->set_service_status (SERVICE_STOPPED, EXIT_FAILURE);
        return;
    } catch (...) {
        this->write_event_log_entry ("unknown error!",
                                     event_type::error);
        this->set_service_status (SERVICE_STOPPED, EXIT_FAILURE);
        return;
    }
    this->write_event_log_entry ("worker thread exiting",
                                 event_type::information);
    this->set_service_status (SERVICE_STOPPED, EXIT_SUCCESS);
}

// stop_handler
// ~~~~~~~~~~~~
/// \note Periodically call ReportServiceStatus() with SERVICE_STOP_PENDING if the procedure is
/// going to take long time.
void broker_service::stop_handler () {
    this->write_event_log_entry ("broker service stopping", event_type::information);
    // Indicate that the service is stopping and join the thread.
    is_stopping_ = true;
}
