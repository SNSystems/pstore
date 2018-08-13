//*                      _            _                     *
//*  ___  ___ _ ____   _(_) ___ ___  | |__   __ _ ___  ___  *
//* / __|/ _ \ '__\ \ / / |/ __/ _ \ | '_ \ / _` / __|/ _ \ *
//* \__ \  __/ |   \ V /| | (_|  __/ | |_) | (_| \__ \  __/ *
//* |___/\___|_|    \_/ |_|\___\___| |_.__/ \__,_|___/\___| *
//*                                                         *
//===- tools/brokerd/service/service_base.cpp -----------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

/// \file service_base.cpp

#include "service_base.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

#if 0
namespace {
    template <typename T, std::ptrdiff_t n>
    constexpr std::ptrdiff_t elements_of (T (&)[n]) {
        return n;
    }
}
#endif


// Initialize the singleton service instance.
service_base * service_base::s_service = nullptr;

// (ctor)
// ~~~~~~
service_base::service_base (TCHAR const * service_name, bool can_stop, bool can_shutdown,
                            bool can_pause_continue)
        : name_{(service_name == nullptr) ? TEXT ("") : service_name} {

    std::memset (&status_, 0, sizeof (status_));
    status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS; // The service runs in its own process.
    status_.dwCurrentState = SERVICE_START_PENDING;    // The service is starting.

    // The accepted commands of the service.
    status_.dwControlsAccepted = 0;
    status_.dwControlsAccepted |= can_stop ? SERVICE_ACCEPT_STOP : 0;
    status_.dwControlsAccepted |= can_shutdown ? SERVICE_ACCEPT_SHUTDOWN : 0;
    status_.dwControlsAccepted |= can_pause_continue ? SERVICE_ACCEPT_PAUSE_CONTINUE : 0;

    status_.dwWin32ExitCode = NO_ERROR;
    status_.dwServiceSpecificExitCode = 0;
    status_.dwCheckPoint = 0;
    status_.dwWaitHint = 0;
}

// (dtor)
// ~~~~~~
service_base::~service_base () {}

//
//   FUNCTION: service_base::Run(service_base &)
//
//   PURPOSE: Register the executable for a service with the Service Control
//   Manager (SCM). After you call Run(ServiceBase), the SCM issues a Start
//   command, which results in a call to the OnStart method in the service.
//   This method blocks until the service has stopped.
//
//   PARAMETERS:
//   * service - the reference to a service_base object. It will become the
//     singleton service instance of this service application.
//
//   RETURN VALUE: If the function succeeds, the return value is TRUE. If the
//   function fails, the return value is FALSE. To get extended error
//   information, call GetLastError.
//
bool service_base::run (service_base & service) {
    s_service = &service;

    /// Make a non-const copy of the servicd name as equired by the definition of
    /// SERVICE_TABLE_ENTRY.
    auto const & n = service.name ();
    std::vector<TCHAR> name (std::begin (n), std::end (n));
    name.push_back (L'\0');

    SERVICE_TABLE_ENTRY const serviceTable[] = {{name.data (), service_base::main},
                                                {nullptr, nullptr}};

    // Connects the main thread of a service process to the service control
    // manager, which causes the thread to be the service control dispatcher
    // thread for the calling process. This call returns when the service has
    // stopped. The process should simply terminate when the call returns.
    return ::StartServiceCtrlDispatcher (serviceTable) != FALSE;
}


// main
// ~~~~
/// Service entry point.
void WINAPI service_base::main (DWORD argc, TCHAR * argv[]) {
    assert (s_service != nullptr);
    if (auto * const service = s_service) {
        // Register the handler function for the service
        service->status_handle_ =
            ::RegisterServiceCtrlHandlerW (service->name_.c_str (), control_handler);
        if (service->status_handle_ == nullptr) {
            throw GetLastError (); // FIXME FFS
        }

        service->start (argc, argv);
    }
}

// control_handler [static]
// ~~~~~~~~~~~~~~~
/// Called by the SCM whenever a control code is sent to the service.
void WINAPI service_base::control_handler (DWORD control_code) {
    assert (s_service != nullptr);
    if (auto * const service = s_service) {
        switch (control_code) {
        case SERVICE_CONTROL_STOP: service->stop (); break;
        case SERVICE_CONTROL_PAUSE: service->pause (); break;
        case SERVICE_CONTROL_CONTINUE: service->resume (); break;
        case SERVICE_CONTROL_SHUTDOWN: service->shutdown (); break;
        case SERVICE_CONTROL_INTERROGATE: break;
        default: assert (false); break;
        }
    }
}

// start
// ~~~~~
/// Starts the service. It calls the start_handler function in which a derived class can perform the
/// real application start.
/// If an error occurs during the startup, the error will be logged in the Application event log,
/// and the service stopped.
///
/// \param argc  Number of command line arguments.
/// \param argv  Array of command line arguments.
void service_base::start (DWORD argc, TCHAR * argv[]) {
    try {
        this->set_service_status (SERVICE_START_PENDING);
        this->start_handler (argc, argv);
        this->set_service_status (SERVICE_RUNNING);
    } catch (DWORD dwError) { // FIXME: FFS
        this->write_error_log_entry ("Service Start", dwError);
        this->set_service_status (SERVICE_STOPPED, dwError);
    } catch (...) {
        this->write_event_log_entry ("Service failed to start.", event_type::error);
        this->set_service_status (SERVICE_STOPPED);
    }
}

// start_handler
// ~~~~~~~~~~~~~
/// Called when a Start command is sent to the service by the SCM or when the operating system
/// starts (for a service that starts automatically). Specifies actions to take when the service
/// starts. Periodically call service_base::set_service_status() with SERVICE_START_PENDING.
///
/// \param argc  Number of command line arguments.
/// \param argv  Array of command line arguments.
void service_base::start_handler (DWORD /*argc*/, TCHAR * /*argv*/ []) {}

// stop
// ~~~~
/// Stops the service.
void service_base::stop () {
    auto const original_state = status_.dwCurrentState;
    try {
        this->set_service_status (SERVICE_STOP_PENDING);
        this->stop_handler ();
        this->set_service_status (SERVICE_STOPPED);
    } catch (DWORD dwError) {
        this->write_error_log_entry ("Service Stop", dwError);
        this->set_service_status (original_state);
    } catch (...) {
        this->write_event_log_entry ("Service failed to stop.", event_type::error);
        this->set_service_status (original_state);
    }
}

// stop_handler
// ~~~~~~~~~~~~
//
//   FUNCTION: service_base::stop_handler()
//
//   PURPOSE: When implemented in a derived class, executes when a Stop
//   command is sent to the service by the SCM. Specifies actions to take
//   when a service stops running. Be sure to periodically call
//   service_base::set_service_status() with SERVICE_STOP_PENDING if the
//   procedure is going to take long time.
//
void service_base::stop_handler () {}


// pause
// ~~~~~
// The function pauses the service if the service supports pause
//   and continue. It calls the pause_handler virtual function in which you can
//   specify the actions to take when the service pauses. If an error occurs,
//   the error will be logged in the Application event log, and the service
//   will become running.
//
void service_base::pause () {
    try {
        this->set_service_status (SERVICE_PAUSE_PENDING);
        this->pause_handler ();
        this->set_service_status (SERVICE_PAUSED);
    } catch (DWORD dwError) { // FIXME
        this->write_error_log_entry ("Service Pause", dwError);
        this->set_service_status (SERVICE_RUNNING);
    } catch (...) {
        this->write_event_log_entry ("Service failed to pause.", event_type::error);
        this->set_service_status (SERVICE_RUNNING);
    }
}


// pause_handler
// ~~~~~~~~~~~~~
/// Called when a pause command is sent to the service by the SCM.
//
void service_base::pause_handler () {}

// resume
// ~~~~~~
// The function resumes normal functioning after being paused if
//   the service supports pause and continue. It calls the resume_handler virtual
//   function in which you can specify the actions to take when the service
//   continues. If an error occurs, the error will be logged in the
//   Application event log, and the service will still be paused.
//
void service_base::resume () {
    try {
        this->set_service_status (SERVICE_CONTINUE_PENDING);
        this->resume_handler ();
        this->set_service_status (SERVICE_RUNNING);
    } catch (DWORD dwError) { // FIXME: FFS
        this->write_error_log_entry ("Service Continue", dwError);
        this->set_service_status (SERVICE_PAUSED);
    } catch (...) {
        this->write_event_log_entry ("Service failed to resume.", event_type::error);
        this->set_service_status (SERVICE_PAUSED);
    }
}

// resume_handler
// ~~~~~~~~~~~~~~
/// When implemented in a derived class, resume_handler runs when a
//   Continue command is sent to the service by the SCM. Specifies actions to
//   take when a service resumes normal functioning after being paused.
//
void service_base::resume_handler () {}

// shutdown
// ~~~~~~~~
/// Called when the system is shutting down.
void service_base::shutdown () {
    try {
        // Perform service-specific shutdown operations.
        this->shutdown_handler ();
        this->set_service_status (SERVICE_STOPPED);
    } catch (DWORD dwError) {
        this->write_error_log_entry ("Service Shutdown", dwError);
    } catch (...) {
        this->write_event_log_entry ("Service failed to shut down.", event_type::error);
    }
}

// set_service_status
// ~~~~~~~~~~~~~~~~~~
void service_base::set_service_status (DWORD current_state, DWORD win32_exit_code,
                                       DWORD wait_hint) noexcept {
    static auto check_point = DWORD{0};

    status_.dwCurrentState = current_state;
    status_.dwWin32ExitCode = win32_exit_code;
    status_.dwWaitHint = wait_hint;
    status_.dwCheckPoint =
        (current_state == SERVICE_RUNNING || current_state == SERVICE_STOPPED) ? 0 : ++check_point;

    // Report the status of the service to the SCM.
    ::SetServiceStatus (status_handle_, &status_);
}

// write_event_log_entry
// ~~~~~~~~~~~~~~~~~~~~~
/// Logs a message to the application event log.
/// \param message  The message to be logged.
/// \param type  The type of event to be logged.
void service_base::write_event_log_entry (char const * message, event_type type) {
    std::array<TCHAR const *, 2> lpszStrings = {{nullptr}};

    TCHAR const pszMessage[] = TEXT ("it happened"); // FIXME: Convert message to UTF16.
    // TODO: shouldn't this be open for longer?
    if (HANDLE event_source = ::RegisterEventSource (NULL, name_.c_str ())) {
        auto t = WORD{0};
        switch (type) {
        case event_type::success: t = EVENTLOG_SUCCESS; break;
        case event_type::audit_failure: t = EVENTLOG_AUDIT_FAILURE; break;
        case event_type::audit_success: t = EVENTLOG_AUDIT_SUCCESS; break;
        case event_type::error: t = EVENTLOG_ERROR_TYPE; break;
        case event_type::information: t = EVENTLOG_INFORMATION_TYPE; break;
        case event_type::warning: t = EVENTLOG_WARNING_TYPE; break;
        }

        lpszStrings[0] = name_.c_str ();
        lpszStrings[1] = pszMessage;

        ::ReportEvent (event_source, t,
                       0,                                       // Event category
                       0,                                       // Event identifier
                       NULL,                                    // No security identifier
                       static_cast<WORD> (lpszStrings.size ()), // TODO: narrow_cast
                       0,                                       // No binary data
                       lpszStrings.data (),
                       NULL // No binary data
        );
        ::DeregisterEventSource (event_source);
    }
}

// write_error_log_entry
// ~~~~~~~~~~~~~~~~~~~~~
/// Log an error message to the Application event log.
/// \param func  The function responsible for the error.
/// \param errcode  The error code.
void service_base::write_error_log_entry (char const * func, DWORD errcode) {
    char message[260];
    (void) std::snprintf (message, sizeof (message), "%s failed w/err 0x%08lx", func, errcode);
    message[sizeof (message) - 1] = '\0';
    this->write_event_log_entry (message, event_type::error);
}
// eof: tools/brokerd/service/service_base.cpp