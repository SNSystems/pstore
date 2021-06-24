//===- tools/brokerd/run_broker.hpp -----------------------*- mode: C++ -*-===//
//*                     _               _              *
//*  _ __ _   _ _ __   | |__  _ __ ___ | | _____ _ __  *
//* | '__| | | | '_ \  | '_ \| '__/ _ \| |/ / _ \ '__| *
//* | |  | |_| | | | | | |_) | | | (_) |   <  __/ |    *
//* |_|   \__,_|_| |_| |_.__/|_|  \___/|_|\_\___|_|    *
//*                                                    *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef RUN_BROKER_HPP
#define RUN_BROKER_HPP

struct switches;
int run_broker (switches const & opt);

#endif // RUN_BROKER_HPP
