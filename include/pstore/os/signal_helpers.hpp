//*      _                   _   _          _                      *
//*  ___(_) __ _ _ __   __ _| | | |__   ___| |_ __   ___ _ __ ___  *
//* / __| |/ _` | '_ \ / _` | | | '_ \ / _ \ | '_ \ / _ \ '__/ __| *
//* \__ \ | (_| | | | | (_| | | | | | |  __/ | |_) |  __/ |  \__ \ *
//* |___/_|\__, |_| |_|\__,_|_| |_| |_|\___|_| .__/ \___|_|  |___/ *
//*        |___/                             |_|                   *
//===- include/pstore/os/signal_helpers.hpp -------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/// \file signal_helpers.hpp

#ifndef PSTORE_OS_SIGNAL_HELPERS_HPP
#define PSTORE_OS_SIGNAL_HELPERS_HPP

#include <cerrno>
#include <csignal>

namespace pstore {

    // A simple RAII class which preserves the value of errno. It's needed in the signal handler to
    // ensure that we don't splat the value that the in-flight code may depend upon.
    class errno_saver {
    public:
        errno_saver () noexcept
                : old_{errno} {
            errno = 0;
        }
        errno_saver (errno_saver const &) = delete;
        ~errno_saver () noexcept { errno = old_; }
        errno_saver & operator= (errno_saver const &) = delete;

    private:
        int const old_;
    };

    using signal_function = void (*) (int);
    signal_function register_signal_handler (int signo, signal_function func);

} // namespace pstore

#endif // PSTORE_OS_SIGNAL_HELPERS_HPP
