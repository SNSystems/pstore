//*                      _            _           _        _ _            *
//*  ___  ___ _ ____   _(_) ___ ___  (_)_ __  ___| |_ __ _| | | ___ _ __  *
//* / __|/ _ \ '__\ \ / / |/ __/ _ \ | | '_ \/ __| __/ _` | | |/ _ \ '__| *
//* \__ \  __/ |   \ V /| | (_|  __/ | | | | \__ \ || (_| | | |  __/ |    *
//* |___/\___|_|    \_/ |_|\___\___| |_|_| |_|___/\__\__,_|_|_|\___|_|    *
//*                                                                       *
//===- tools/brokerd/service/service_installer.cpp ------------------------===//
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

/// \file service_installer.cpp

#include "service_installer.hpp"

#include <chrono>
#include <cstdio>
#include <thread>

namespace {

    class service_handle {
    public:
        explicit service_handle (SC_HANDLE h);
        ~service_handle ();
        service_handle (service_handle const &) = delete;
        service_handle & operator= (service_handle const &) = delete;

        SC_HANDLE get () const noexcept { return h_; }

    private:
        SC_HANDLE h_;
    };

    service_handle::service_handle (SC_HANDLE h)
            : h_{h} {}

    service_handle::~service_handle () {
        if (h_) {
            ::CloseServiceHandle (h_);
            h_ = nullptr;
        }
    }

} // namespace

// install_service
// ~~~~~~~~~~~~~~~
void install_service (ctzstring service_name, ctzstring display_name, DWORD start_type,
                      ctzstring dependencies, ctzstring account, ctzstring password) {
    TCHAR szPath[MAX_PATH]; // FIXME: eliminate MAX_PATH. Just because the OS is dumb, our code
                            // doesn't have to be...

    if (::GetModuleFileName (NULL, szPath, ARRAYSIZE (szPath)) == 0) {
        wprintf (TEXT ("GetModuleFileName failed w/err 0x%08lx\n"), GetLastError ());
        throw "FIXME";
    }

    // Open the local default service control manager database
    service_handle scm{
        ::OpenSCManager (NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE)};
    if (scm.get () == NULL) {
        wprintf (TEXT ("OpenSCManager failed w/err 0x%08lx\n"), GetLastError ());
        throw "FIXME";
    }

    // Install the service into SCM.
    service_handle service{::CreateService (scm.get (), // SCManager database
                                            service_name,
                                            display_name,              // Name to display
                                            SERVICE_QUERY_STATUS,      // Desired access
                                            SERVICE_WIN32_OWN_PROCESS, // Service type
                                            start_type,                // Service start type
                                            SERVICE_ERROR_NORMAL,      // Error control type
                                            szPath,                    // Service's binary
                                            NULL,                      // No load ordering group
                                            NULL,                      // No tag identifier
                                            dependencies,              // Dependencies
                                            account,                   // Service running account
                                            password)};
    if (service.get () == nullptr) {
        wprintf (TEXT ("CreateService failed w/err 0x%08lx\n"), GetLastError ());
        throw "FIXME";
    }

    wprintf (TEXT ("%s is installed.\n"), service_name);
}

// uninstall_service
// ~~~~~~~~~~~~~~~~~
/// Stop and remove the service from the local service control manager database.
/// \param service_name  The name of the service to be removed.
void uninstall_service (TCHAR const * service_name) {

    // Open the local default service control manager database
    service_handle scm{::OpenSCManager (NULL, NULL, SC_MANAGER_CONNECT)};
    if (scm.get () == nullptr) {
        wprintf (TEXT ("OpenSCManager failed w/err 0x%08lx\n"), GetLastError ());
        throw "FIXME";
    }

    // Open the service with delete, stop, and query status permissions
    service_handle service{
        ::OpenService (scm.get (), service_name, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE)};
    if (service.get () == nullptr) {
        wprintf (TEXT ("OpenService failed w/err 0x%08lx\n"), GetLastError ());
        throw "FIXME";
    }

    // Try to stop the service
    SERVICE_STATUS status{};
    if (::ControlService (service.get (), SERVICE_CONTROL_STOP, &status)) {
        using namespace std::chrono_literals;

        // FIXME: can I use a condition variable rather than polling?
        wprintf (TEXT ("Stopping %s."), service_name);
        std::this_thread::sleep_for (1s);

        while (::QueryServiceStatus (service.get (), &status)) {
            if (status.dwCurrentState == SERVICE_STOP_PENDING) {
                wprintf (L".");
                std::this_thread::sleep_for (1s);
            } else {
                break;
            }
        }

        if (status.dwCurrentState == SERVICE_STOPPED) {
            wprintf (TEXT ("\n%s is stopped.\n"), service_name);
        } else {
            wprintf (TEXT ("\n%s failed to stop.\n"), service_name);
        }
    }

    if (!::DeleteService (service.get ())) {
        wprintf (TEXT ("DeleteService failed w/err 0x%08lx\n"), GetLastError ());
        throw "FIXME";
    }
}
