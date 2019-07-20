//*  _               _                                   _           *
//* | |__  _ __ ___ | | _____ _ __   ___  ___ _ ____   _(_) ___ ___  *
//* | '_ \| '__/ _ \| |/ / _ \ '__| / __|/ _ \ '__\ \ / / |/ __/ _ \ *
//* | |_) | | | (_) |   <  __/ |    \__ \  __/ |   \ V /| | (_|  __/ *
//* |_.__/|_|  \___/|_|\_\___|_|    |___/\___|_|    \_/ |_|\___\___| *
//*                                                                  *
//===- tools/brokerd/service/broker_service.cpp ---------------------------===//
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
    thread_ = std::thread ([this]() { this->worker (); });
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
