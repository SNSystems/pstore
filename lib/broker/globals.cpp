//===- lib/broker/globals.cpp ---------------------------------------------===//
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
/// \file globals.cpp

#include "pstore/broker/globals.hpp"
#include <cstdlib>

namespace pstore {
    namespace broker {

        std::atomic<bool> done;
        std::atomic<int> exit_code (EXIT_SUCCESS);

        std::mutex iomut;

    } // namespace broker
} // namespace pstore
