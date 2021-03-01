//===- include/pstore/broker/globals.hpp ------------------*- mode: C++ -*-===//
//*        _       _           _      *
//*   __ _| | ___ | |__   __ _| |___  *
//*  / _` | |/ _ \| '_ \ / _` | / __| *
//* | (_| | | (_) | |_) | (_| | \__ \ *
//*  \__, |_|\___/|_.__/ \__,_|_|___/ *
//*  |___/                            *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file globals.hpp

#ifndef PSTORE_BROKER_GLOBALS_HPP
#define PSTORE_BROKER_GLOBALS_HPP

#include <atomic>
#include <mutex>

namespace pstore {
    namespace broker {

        extern std::atomic<bool> done;
        extern std::atomic<int> exit_code;

        /// A mutex to guard any use of stdout or stderr.
        extern std::mutex iomut;

    } // namespace broker
} // namespace pstore

#endif // PSTORE_BROKER_GLOBALS_HPP
