//===- include/pstore/vacuum/status.hpp -------------------*- mode: C++ -*-===//
//*      _        _              *
//*  ___| |_ __ _| |_ _   _ ___  *
//* / __| __/ _` | __| | | / __| *
//* \__ \ || (_| | |_| |_| \__ \ *
//* |___/\__\__,_|\__|\__,_|___/ *
//*                              *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Copyright (c) 2015-2016 by Sony Interactive Entertainment, Inc.
/// \file status.hpp

#ifndef VACUUM_STATUS_HPP
#define VACUUM_STATUS_HPP (1)

#include <atomic>
#include <chrono>

namespace vacuum {

    struct status {
        std::atomic<bool> modified{false};
        std::atomic<bool> done{false};
        std::atomic<bool> watch_running{false};
    };

    auto constexpr initial_delay = std::chrono::seconds (10);
    auto constexpr watch_interval = std::chrono::milliseconds (500);

} // namespace vacuum
#endif // VACUUM_STATUS_HPP
