//*      _             _                                            *
//*  ___| |_ __ _ _ __| |_  __   ____ _  ___ _   _ _   _ _ __ ___   *
//* / __| __/ _` | '__| __| \ \ / / _` |/ __| | | | | | | '_ ` _ \  *
//* \__ \ || (_| | |  | |_   \ V / (_| | (__| |_| | |_| | | | | | | *
//* |___/\__\__,_|_|   \__|   \_/ \__,_|\___|\__,_|\__,_|_| |_| |_| *
//*                                                                 *
//===- include/pstore/core/start_vacuum.hpp -------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file start_vacuum.hpp

#ifndef PSTORE_CORE_START_VACUUM_HPP
#define PSTORE_CORE_START_VACUUM_HPP

namespace pstore {

    class database;
    void start_vacuum (database const & db);

} // namespace pstore

#endif // PSTORE_CORE_START_VACUUM_HPP
