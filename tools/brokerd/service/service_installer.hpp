//*                      _            _           _        _ _            *
//*  ___  ___ _ ____   _(_) ___ ___  (_)_ __  ___| |_ __ _| | | ___ _ __  *
//* / __|/ _ \ '__\ \ / / |/ __/ _ \ | | '_ \/ __| __/ _` | | |/ _ \ '__| *
//* \__ \  __/ |   \ V /| | (_|  __/ | | | | \__ \ || (_| | | |  __/ |    *
//* |___/\___|_|    \_/ |_|\___\___| |_|_| |_|___/\__\__,_|_|_|\___|_|    *
//*                                                                       *
//===- tools/brokerd/service/service_installer.hpp ------------------------===//
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

/// \file service_installer.hpp

#ifndef PSTORE_BROKER_SERVICE_INSTALLER_HPP
#define PSTORE_BROKER_SERVICE_INSTALLER_HPP

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "pstore/support/gsl.hpp"

using tzstring = ::pstore::gsl::basic_zstring<TCHAR>;
using ctzstring = ::pstore::gsl::basic_zstring<TCHAR const>;



/// \brief  Install the current application as a service to the local service control manager
/// database.
///
/// \param service_name  The name of the service to be installed
/// \param display_name  The display name of the service.
/// \param start_type  The service start option. One of: SERVICE_AUTO_START, SERVICE_BOOT_START,
/// SERVICE_DEMAND_START, SERVICE_DISABLED, SERVICE_SYSTEM_START.
/// \param dependencies  A pointer to a double null-terminated array of null-separated names of
/// services or load ordering groups that the system must start before this service. [See
/// CreateService() documentation.]
/// \param account  The name of the account under which the service runs.
/// \param password  The password for the account under which the service runs.
void install_service (ctzstring service_name, ctzstring display_name, DWORD start_type,
                      ctzstring dependencies, ctzstring account, ctzstring password);


/// Stop and remove the service from the local service control manager database.
///
/// \param service_name  The name of the service to be removed.
void uninstall_service (ctzstring service_name);

#endif // PSTORE_BROKER_SERVICE_INSTALLER_HPP
