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

#include "broker_service.hpp"

#include "../../run_broker.hpp"
#include "../../switches.hpp"

#include "pstore/broker/globals.hpp"
#include "pstore/broker/quit.hpp"
#include "pstore/command_line/option.hpp"

using namespace std::string_literals;
using namespace pstore;

// (ctor)
// ~~~~~~
broker_service::broker_service (TCHAR const * service_name, bool can_stop, bool can_shutdown,
                                bool can_pause_continue)
        : service_base (service_name, can_stop, can_shutdown, can_pause_continue) {}

// (dtor)
// ~~~~~~
broker_service::~broker_service () = default;

// start_handler
// ~~~~~~~~~~~~~
/// \brief Executed when a 'start' command is sent to the service by the SCM or when the operating
/// system starts (for a service that starts automatically).
/// \note start_handler() must return to the operating system after the service's operation has
/// begun: it must not block.

void broker_service::start_handler (DWORD argc, TCHAR * argv[]) {
    this->write_event_log_entry ("broker service starting", event_type::information);

    command_line::option::reset_container ();

    switches opt;
    std::tie (opt, broker::exit_code) = get_switches (argc, argv);

    if (broker::exit_code != EXIT_SUCCESS) {
        this->write_event_log_entry ("error: broker service failed to parse commandline options",
                                     event_type::error);
        this->set_service_status (SERVICE_STOPPED, EXIT_FAILURE);
        return;
    }

    this->worker_thread_ = std::thread ([this, opt] { this->worker (opt); });

    this->write_event_log_entry ("broker service started successfully", event_type::information);
}

// worker
// ~~~~~~
void broker_service::worker (switches opt) {
    try {
        run_broker (opt);
        if (broker::exit_code != EXIT_SUCCESS) {
            this->write_event_log_entry ("broker service exited unsuccessfully", event_type::error);
        }
    } catch (std::exception const & ex) {
        this->write_event_log_entry (std::string ("error: ", ex.what ()).c_str (),
                                     event_type::error);
        this->set_service_status (SERVICE_STOPPED, EXIT_FAILURE);
    } catch (...) {
        this->write_event_log_entry ("unknown error!", event_type::error);
        this->set_service_status (SERVICE_STOPPED, EXIT_FAILURE);
    }
}

// stop_handler
// ~~~~~~~~~~~~
/// \note Periodically call ReportServiceStatus() with SERVICE_STOP_PENDING if the procedure is
/// going to take long time.
void broker_service::stop_handler () {
    this->write_event_log_entry ("broker quiting", event_type::information);
    broker::notify_quit_thread ();
    this->worker_thread_.join ();
    this->write_event_log_entry ("broker threads quit successfully", event_type::information);
}
