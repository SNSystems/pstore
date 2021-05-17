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

using namespace std::string_literals;
using namespace pstore;

extern romfs::romfs fs;

// (ctor)
// ~~~~~~
broker_service::broker_service (TCHAR const * service_name, bool can_stop, bool can_shutdown,
                                bool can_pause_continue)
        : service_base (service_name, can_stop, can_shutdown, can_pause_continue) {}

// (dtor)
// ~~~~~~
broker_service::~broker_service () {}

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

    broker::create_thread ([this, opts] { this->worker (opts.first); });
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
        maybe<http::server_status> http_status = broker::get_http_server_status (opt.http_port);

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
        broker::create_http_worker_thread (&futures, &http_status, opt.num_read_threads, fs);

        for (auto ctr = 0U; ctr < opt.num_read_threads; ++ctr) {
            futures.push_back (broker::create_thread ([ctr, &fifo, &record_file, commands] () {
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

        this->write_event_log_entry ("worker threads done, stopping quit thread",
                                     event_type::information);
        broker::notify_quit_thread ();
        quit.join ();

    } catch (std::exception const & ex) {
        this->write_event_log_entry (std::string ("error: ", ex.what ()).c_str (),
                                     event_type::error);
        this->set_service_status (SERVICE_STOPPED, EXIT_FAILURE);
        return;
    } catch (...) {
        this->write_event_log_entry ("unknown error!", event_type::error);
        this->set_service_status (SERVICE_STOPPED, EXIT_FAILURE);
        return;
    }
    this->write_event_log_entry ("worker thread exiting", event_type::information);
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
