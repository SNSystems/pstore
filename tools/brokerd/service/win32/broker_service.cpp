//*  _               _                                   _           *
//* | |__  _ __ ___ | | _____ _ __   ___  ___ _ ____   _(_) ___ ___  *
//* | '_ \| '__/ _ \| |/ / _ \ '__| / __|/ _ \ '__\ \ / / |/ __/ _ \ *
//* | |_) | | | (_) |   <  __/ |    \__ \  __/ |   \ V /| | (_|  __/ *
//* |_.__/|_|  \___/|_|\_\___|_|    |___/\___|_|    \_/ |_|\___\___| *
//*                                                                  *
//===- tools/brokerd/service/win32/broker_service.cpp ---------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "broker_service.hpp"

// (ctor)
// ~~~~~~
sample_service::sample_service (TCHAR const * service_name, bool can_stop, bool can_shutdown,
                                bool can_pause_continue)
        : service_base (service_name, can_stop, can_shutdown, can_pause_continue) {}

// (dtor)
// ~~~~~~
sample_service::~sample_service () {
    if (thread_.joinable ()) {
        thread_.detach ();
    }
}

// start_handler
// ~~~~~~~~~~~~~
/// \brief Executed when a 'start' command is sent to the service by the SCM or when the operating
/// system starts (for a service that starts automatically).
/// \note start_handler() must return to the operating system after the service's operation has
/// begun: it must not block.

void sample_service::start_handler (DWORD /*argc*/, TCHAR * /*argv*/[]) {
    // Log a service start message to the Application log.
    this->write_event_log_entry ("CppWindowsService in OnStart", event_type::information);
    thread_ = std::thread ([this] () { this->worker (); });
}


// worker
// ~~~~~~
/// The method performs the main function of the service. It runs on a thread pool worker thread.
void sample_service::worker () {
    // Periodically check if the service is stopping.
    while (!is_stopping_) {
        // Perform main service function here...
        using namespace std::chrono_literals;

        wprintf (TEXT ("Stopping %s."), this->name ().c_str ());
        std::this_thread::sleep_for (2s); // Simulate some lengthy operations.
    }
}

// stop_handler
// ~~~~~~~~~~~~
/// \note Periodically call ReportServiceStatus() with SERVICE_STOP_PENDING if the procedure is
/// going to take long time.
void sample_service::stop_handler () {
    this->write_event_log_entry ("CppWindowsService in stop_handler", event_type::information);

    // Indicate that the service is stopping and join the thread.
    is_stopping_ = true;
    thread_.join ();
}
