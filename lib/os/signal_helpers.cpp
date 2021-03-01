//===- lib/os/signal_helpers.cpp ------------------------------------------===//
//*      _                   _   _          _                      *
//*  ___(_) __ _ _ __   __ _| | | |__   ___| |_ __   ___ _ __ ___  *
//* / __| |/ _` | '_ \ / _` | | | '_ \ / _ \ | '_ \ / _ \ '__/ __| *
//* \__ \ | (_| | | | | (_| | | | | | |  __/ | |_) |  __/ |  \__ \ *
//* |___/_|\__, |_| |_|\__,_|_| |_| |_|\___|_| .__/ \___|_|  |___/ *
//*        |___/                             |_|                   *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/// \file signal_helpers.cpp

#include "pstore/os/signal_helpers.hpp"

#include "pstore/support/error.hpp"

#ifdef _WIN32

namespace pstore {

    // register_signal_handler
    // ~~~~~~~~~~~~~~~~~~~~~~~
    signal_function register_signal_handler (int const signo, signal_function const func) {
        auto res = signal (signo, func);
        if (signal (signo, func) == SIG_ERR) {
            raise (std::errc::invalid_argument, "sigaction error");
        }
        return res;
    }

} // end namespace pstore

#else

namespace pstore {

    // register_signal_handler
    // ~~~~~~~~~~~~~~~~~~~~~~~
    signal_function register_signal_handler (int const signo, signal_function const func) {
        struct sigaction act;
        act.sa_handler = func;
        sigemptyset (&act.sa_mask);
        act.sa_flags = 0;
#    ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#    endif
        struct sigaction oact;
        if (sigaction (signo, &act, &oact) < 0) {
            raise (std::errc::invalid_argument, "sigaction error");
        }
        return oact.sa_handler;
    }

} // end namespace pstore

#endif // _WIN32
