//*                                          _       _    __  *
//* __   ____ _  ___ _   _ _   _ _ __ ___   (_)_ __ | |_ / _| *
//* \ \ / / _` |/ __| | | | | | | '_ ` _ \  | | '_ \| __| |_  *
//*  \ V / (_| | (__| |_| | |_| | | | | | | | | | | | |_|  _| *
//*   \_/ \__,_|\___|\__,_|\__,_|_| |_| |_| |_|_| |_|\__|_|   *
//*                                                           *
//===- include/pstore/core/vacuum_intf.hpp --------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file vacuum_intf.hpp

#ifndef PSTORE_CORE_VACUUM_INTF_HPP
#define PSTORE_CORE_VACUUM_INTF_HPP

#include <atomic>
#include <cstdint>

#include <ctime>

#if defined(_WIN32)
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#else
#    include <pthread.h>
#    include <unistd.h>
#endif

namespace pstore {

#if defined(_WIN32)
    using pid_t = DWORD;
#else
    using pid_t = ::pid_t;
#endif // !defined (_WIN32)

    struct shared {
        static constexpr auto not_running = pid_t{0};
        static constexpr auto starting = static_cast<pid_t> (-1);

        shared ();

        std::atomic<pid_t> pid{0};
        /// The time at which the process was started, in milliseconds since the epoch.
        std::atomic<std::uint64_t> start_time{0};
        std::atomic<std::time_t> time{0};

        /// A value which is periodically incremented whilst a pstore instance is open on the
        /// system.
        /// This can be used to detect that the pstore is in use by another process.
        std::atomic<std::uint64_t> open_tick;
    };

} // namespace pstore
#endif // PSTORE_CORE_VACUUM_INTF_HPP
