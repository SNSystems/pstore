//===- tools/brokerd/service/win32/service_base.hpp -------*- mode: C++ -*-===//
//*                      _            _                     *
//*  ___  ___ _ ____   _(_) ___ ___  | |__   __ _ ___  ___  *
//* / __|/ _ \ '__\ \ / / |/ __/ _ \ | '_ \ / _` / __|/ _ \ *
//* \__ \  __/ |   \ V /| | (_|  __/ | |_) | (_| \__ \  __/ *
//* |___/\___|_|    \_/ |_|\___\___| |_.__/ \__,_|___/\___| *
//*                                                         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/// \file service_base.hpp

#ifndef PSTORE_BROKER_SERVICE_BASE_HPP
#define PSTORE_BROKER_SERVICE_BASE_HPP

#include <string>
#include <system_error>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class service_base {
public:
    /// Registers the executable for a service with the Service Control Manager (SCM). After you
    /// call run(service_base), the SCM issues a Start command, which results in a call to the
    /// OnStart method in the service. This method blocks until the service has stopped.
    ///
    /// \param service  A reference to a service_base object. It will become the singleton service
    /// instance of this service application.
    static void run (service_base & service);

    /// \param service_name  The name of the service.
    /// \param can_stop  Can the service be stopped?
    /// \param can_shutdown  Should the service be notified when system shutdown occurs?
    /// \param can_pause_continue  Can the service be paused and continued?
    explicit service_base (TCHAR const * service_name, bool can_stop = true,
                           bool can_shutdown = true, bool can_pause_continue = false);

    virtual ~service_base ();

    /// Stops the service.
    void stop ();

    std::wstring const & name () const { return name_; }

protected:
    /// Called when a start command is sent to the service by the SCM or when the operating system
    /// starts (for a service that starts automatically).
    virtual void start_handler (DWORD argc, TCHAR * argv[]);

    /// Called when a Stop command is sent to the service by the SCM.
    /// \note Periodically call ReportServiceStatus() with SERVICE_STOP_PENDING if the procedure is
    /// going to take long time.
    virtual void stop_handler ();

    /// Called when a pause command is sent to the service by the SCM.
    virtual void pause_handler ();

    /// Called when a continue command is received from the SCM. Implements any behaviour that is
    /// requied when a service resumes normal functioning after being paused.
    virtual void resume_handler ();

    /// Implement in a derived class to handle system shutdown.
    virtual void shutdown_handler () {}

    /// Set the service status and report the status to the SCM.
    /// \param current_state  The state of the service.
    /// \param win32_exit_code  Error code to report.
    /// \param wait_hint  Estimated time for pending operation, in milliseconds.
    void set_service_status (DWORD current_state, DWORD win32_exit_code = NO_ERROR,
                             DWORD wait_hint = 0) noexcept;

    enum class event_type { success, audit_failure, audit_success, error, information, warning };

    /// Logs a message to the application event log.
    /// \param message  The message to be logged.
    /// \param type  The type of event to be logged.
    void write_event_log_entry (char const * message, event_type type);

    /// Log an error message to the Application event log.
    /// \param message  The message to be logged.
    /// \param sys_ex  The error code.
    void write_error_log_entry (char const * message, std::system_error const & sys_ex);
    /// Log an error message to the Application event log.
    /// \param message  The message to be logged.
    /// \param ex  The error code.
    void write_error_log_entry (char const * message, std::exception const & ex);

private:
    // Entry point for the service. It registers the handler function for the
    // service and starts the service.
    static void WINAPI main (DWORD argc, TCHAR * argv[]);

    // The function is called by the SCM whenever a control code is sent to
    // the service.
    static void WINAPI control_handler (DWORD control_code);

    void start (DWORD argc, TCHAR * argv[]);
    void pause ();
    void resume ();
    void shutdown ();

    // The singleton service instance.
    static service_base * s_service;

    std::wstring const name_;                       ///< The name of the service.
    SERVICE_STATUS status_;                         ///< The status of the service.
    SERVICE_STATUS_HANDLE status_handle_ = nullptr; ///< The service status handle.
};
#endif // PSTORE_BROKER_SERVICE_BASE_HPP
