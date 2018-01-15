//*                                  *
//*  ___ _ __   __ ___      ___ __   *
//* / __| '_ \ / _` \ \ /\ / / '_ \  *
//* \__ \ |_) | (_| |\ V  V /| | | | *
//* |___/ .__/ \__,_| \_/\_/ |_| |_| *
//*     |_|                          *
//===- lib/broker/spawn_posix.cpp -----------------------------------------===//
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
/// \file spawn_posix.cpp

#include "broker/spawn.hpp"

#ifndef _WIN32

// standard includes
#include <cerrno>
#include <cstdlib>
// platform includes
#include <unistd.h>

#include "pstore_support/error.hpp"
#include "pstore_support/logging.hpp"

namespace broker {

    pid_t spawn (::pstore::gsl::czstring exe_path, ::pstore::gsl::czstring * argv) {

        auto const child_pid = ::fork ();
        switch (child_pid) {
        // When fork() returns -1, an error happened.
        case -1:
            raise (pstore::errno_erc{errno}, "fork");
        // When fork() returns 0, we are in the child process.
        case 0: {
            try {
                pstore::logging::log (pstore::logging::priority::info, "starting vacuum ",
                                      pstore::logging::quoted (exe_path));
                // TODO: nice the child process?
                ::execv (exe_path, const_cast<char **> (argv));

                // If execv returns, it must have failed.
                raise (pstore::errno_erc{errno}, "execv");
            } catch (std::exception const & ex) {
                pstore::logging::log (pstore::logging::priority::error, "fork error: ", ex.what ());
            } catch (...) {
                pstore::logging::log (pstore::logging::priority::error, "fork unknown error");
            }
            std::exit (EXIT_FAILURE);
        }
        default:
            // When fork() returns a positive number, we are in the parent process
            // and the return value is the PID of the newly created child process.
            // There's a remote chance that the process came and went by the time that
            // we got here, so record the child PID if we're still in the "starting"
            // state.

            pstore::logging::log (pstore::logging::priority::info, "vacuum is now running: pid ",
                                  child_pid);
            break;
        }
        return child_pid;
    }

} // namespace broker
#endif //!_WIN32
// eof: lib/broker/spawn_posix.cpp
