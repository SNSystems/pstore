//===- tools/broker_poker/flood_server.hpp ----------------*- mode: C++ -*-===//
//*   __ _                 _                                 *
//*  / _| | ___   ___   __| |  ___  ___ _ ____   _____ _ __  *
//* | |_| |/ _ \ / _ \ / _` | / __|/ _ \ '__\ \ / / _ \ '__| *
//* |  _| | (_) | (_) | (_| | \__ \  __/ |   \ V /  __/ |    *
//* |_| |_|\___/ \___/ \__,_| |___/\___|_|    \_/ \___|_|    *
//*                                                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef PSTORE_BROKER_POKER_FLOOD_SERVER_HPP
#define PSTORE_BROKER_POKER_FLOOD_SERVER_HPP

#include <chrono>
#include "pstore/support/gsl.hpp"

void flood_server (pstore::gsl::czstring pipe_path, std::chrono::milliseconds retry_timeout,
                   unsigned long num);

#endif // PSTORE_BROKER_POKER_FLOOD_SERVER_HPP
