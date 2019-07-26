//*                                          _       _    __  *
//* __   ____ _  ___ _   _ _   _ _ __ ___   (_)_ __ | |_ / _| *
//* \ \ / / _` |/ __| | | | | | | '_ ` _ \  | | '_ \| __| |_  *
//*  \ V / (_| | (__| |_| | |_| | | | | | | | | | | | |_|  _| *
//*   \_/ \__,_|\___|\__,_|\__,_|_| |_| |_| |_|_| |_|\__|_|   *
//*                                                           *
//===- include/pstore/core/vacuum_intf.hpp --------------------------------===//
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
/// \file vacuum_intf.hpp

#ifndef PSTORE_VACUUM_INTF_HPP
#define PSTORE_VACUUM_INTF_HPP (1)

#include <atomic>
#include <cstdint>

#include <ctime>

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <pthread.h>
#include <unistd.h>
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
#endif // PSTORE_VACUUM_INTF_HPP
