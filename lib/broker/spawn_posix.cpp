//===- lib/broker/spawn_posix.cpp -----------------------------------------===//
//*                                  *
//*  ___ _ __   __ ___      ___ __   *
//* / __| '_ \ / _` \ \ /\ / / '_ \  *
//* \__ \ |_) | (_| |\ V  V /| | | | *
//* |___/ .__/ \__,_| \_/\_/ |_| |_| *
//*     |_|                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file spawn_posix.cpp

#include "pstore/broker/spawn.hpp"

#ifndef _WIN32

// Standard includes
#    include <cerrno>
#    include <cstdlib>

// Platform includes
#    include <unistd.h>

// pstore includes
#    include "pstore/os/logging.hpp"
#    include "pstore/support/error.hpp"

namespace pstore {
    namespace broker {

        process_identifier spawn (gsl::czstring const exe_path,
                                  gsl::not_null<gsl::czstring const *> const argv) {
            using priority = logger::priority;
            auto const child_pid = ::fork ();
            switch (child_pid) {
            // When fork() returns -1, an error happened.
            case -1: raise (errno_erc{errno}, "fork");
            // When fork() returns 0, we are in the child process.
            case 0: {
                try {
                    log (priority::info, "starting vacuum ", logger::quoted{exe_path});
                    ::execv (exe_path, const_cast<char **> (argv.get ()));
                    raise (errno_erc{errno}, "execv"); // If execv returns, it must have failed.
                } catch (std::exception const & ex) {
                    log (priority::error, "fork error: ", ex.what ());
                } catch (...) {
                    log (priority::error, "fork unknown error");
                }
                std::exit (EXIT_FAILURE);
            }
            default:
                // When fork() returns a positive number, we are in the parent process
                // and the return value is the PID of the newly created child process.
                // There's a remote chance that the process came and went by the time that
                // we got here, so record the child PID if we're still in the "starting"
                // state.

                log (priority::info, "vacuum is now running: pid ", child_pid);
                break;
            }
            return child_pid;
        }

    } // namespace broker
} // namespace pstore

#endif //!_WIN32
