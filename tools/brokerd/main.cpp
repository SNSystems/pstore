//===- tools/brokerd/main.cpp ---------------------------------------------===//
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

#include "switches.hpp"
#include "brokerd.hpp"

#include "pstore/broker/globals.hpp"
#include "pstore/os/logging.hpp"

using namespace pstore;
using priority = logger::priority;

#if defined(_WIN32)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    switches opt;
    std::tie (opt, broker::exit_code) = get_switches (argc, argv);

    try {
        broker::exit_code = pstore::broker::run_broker (opt);
    } catch (std::exception const & ex) {
        log (priority::error, "error: ", ex.what ());
        broker::exit_code = EXIT_FAILURE;
    } catch (...) {
        log (priority::error, "unknown error");
        broker::exit_code = EXIT_FAILURE;
    }

    return broker::exit_code;
}
