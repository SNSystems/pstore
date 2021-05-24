//===- tools/brokerd/service/win32/main.cpp -------------------------------===//
//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <cstdio>
#include <iostream>
#include <vector>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "pstore/command_line/command_line.hpp"
#include "pstore/support/utf.hpp"

// Local includes
#include "./service_installer.hpp"
#include "./broker_service.hpp"

namespace {

    constexpr auto service_name = TEXT ("pstore_broker");
    constexpr auto display_name = TEXT ("pstore broker");
    constexpr auto start_type = SERVICE_DEMAND_START;
    constexpr auto service_dependencies =
        TEXT (""); // List of service dependencies - "dep1\0dep2\0\0"
    constexpr auto account = TEXT ("NT AUTHORITY\\LocalService");
    constexpr auto account_password = nullptr;


    using namespace pstore::command_line;

    opt<bool> install_opt ("install", desc ("Install the service"));
    opt<bool> remove_opt ("remove", desc ("Remove the service"));

} // end anonymous namespace


int _tmain (int argc, TCHAR * argv[]) {
    int exit_code = EXIT_SUCCESS;
    try {
        parse_command_line_options (argc, argv, "pstore broker server");

        if (install_opt && remove_opt) {
            std::wcerr << L"--install and --remove cannot be specified together!\n";
            std::exit (EXIT_FAILURE);
        }

        if (install_opt) {
            install_service (service_name, display_name, start_type, service_dependencies, account,
                             account_password);
        } else if (remove_opt) {
            uninstall_service (service_name);
        } else {
            broker_service service{service_name};
            broker_service::run (service);
        }
    } catch (std::exception const & ex) {
        std::wcerr << L"Error: " << pstore::utf::to_native_string (ex.what ()) << std::endl;
        exit_code = EXIT_FAILURE;
    } catch (...) {
        std::wcerr << L"Unknown error" << std::endl;
        exit_code = EXIT_FAILURE;
    }

    return exit_code;
}
