//*                      _            _           _        _ _            *
//*  ___  ___ _ ____   _(_) ___ ___  (_)_ __  ___| |_ __ _| | | ___ _ __  *
//* / __|/ _ \ '__\ \ / / |/ __/ _ \ | | '_ \/ __| __/ _` | | |/ _ \ '__| *
//* \__ \  __/ |   \ V /| | (_|  __/ | | | | \__ \ || (_| | | |  __/ |    *
//* |___/\___|_|    \_/ |_|\___\___| |_|_| |_|___/\__\__,_|_|_|\___|_|    *
//*                                                                       *
//===- tools/brokerd/service/win32/service_installer.hpp ------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
