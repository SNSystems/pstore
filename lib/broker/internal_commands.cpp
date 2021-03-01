//===- lib/broker/internal_commands.cpp -----------------------------------===//
//*  _       _                        _  *
//* (_)_ __ | |_ ___ _ __ _ __   __ _| | *
//* | | '_ \| __/ _ \ '__| '_ \ / _` | | *
//* | | | | | ||  __/ |  | | | | (_| | | *
//* |_|_| |_|\__\___|_|  |_| |_|\__,_|_| *
//*                                      *
//*                                                _      *
//*   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| |___  *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` / __| *
//* | (_| (_) | | | | | | | | | | | (_| | | | | (_| \__ \ *
//*  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_|___/ *
//*                                                       *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/broker/internal_commands.hpp"

namespace pstore {
    namespace broker {

        gsl::czstring const read_loop_quit_command = "_QUIT";
        gsl::czstring const command_loop_quit_command = "_CQUIT";

    } // end namespace broker
} // end namespace pstore
