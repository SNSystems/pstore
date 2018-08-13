//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===- tools/brokerd/service/main.cpp -------------------------------------===//
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
#include <cstdio>
#include <iostream>
#include <vector>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "pstore/cmd_util/cl/command_line.hpp"
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
} // namespace


namespace {
    using namespace pstore::cmd_util;

    cl::opt<bool> install_opt ("install", cl::desc ("Install the service"));
    cl::opt<bool> remove_opt ("remove", cl::desc ("Remove the service"));

#if 0
    enum option_index {
        help_opt,
        install_opt,
        remove_opt,
        unknown_opt,
    };

#define USAGE TEXT ("pstore_broker_serivce [options]")

    option::Descriptor const usage[] = {
        {unknown_opt, 0, TEXT (""), TEXT (""), option::Arg::None, USAGE "\n\nOptions:"},
        {help_opt, 0, TEXT (""), TEXT ("help"), option::Arg::None,
         TEXT ("  --help \tPrint usage and exit.")},

        {install_opt, 0, TEXT ("i"), TEXT ("install"), option::Arg::None,
         TEXT ("  --install, -i  \tInstall the service.")},
        {install_opt, 0, TEXT ("r"), TEXT ("remove"), option::Arg::None,
         TEXT ("  --remove, -r  \tRemovethe service.")},

        {0, 0, 0, 0, 0, 0},
    };
#endif
} // namespace


int _tmain (int argc, TCHAR * argv[]) {
    int exit_code = EXIT_SUCCESS;
    try {
        cl::ParseCommandLineOptions (argc, argv, "pstore broker server");

        if (install_opt && remove_opt) {
            std::cerr << "--install and --remove cannot be specified together!\n";
            std::exit (EXIT_FAILURE);
        }

        if (install_opt) {
            install_service (service_name, display_name, start_type, service_dependencies, account,
                             account_password);
        } else if (remove_opt) {
            uninstall_service (service_name);
        } else {
            sample_service service (service_name);
            if (!sample_service::run (service)) {
                wprintf (TEXT ("Service failed to run w/err 0x%08lx\n"), ::GetLastError ());
            }
        }
    } catch (std::exception const & ex) {
        char const * what = ex.what ();
        std::cerr << "Error: " << what << std::endl; // FIXME: use wide characters
        exit_code = EXIT_FAILURE;
    } catch (...) {
        std::wcerr << TEXT ("Unknown error") << std::endl;
        exit_code = EXIT_FAILURE;
    }

    return exit_code;
}
// eof: tools/brokerd/service/main.cpp