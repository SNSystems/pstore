//*                           _             _                *
//* __      _____  __ _   ___| |_ __ _ _ __| |_ _   _ _ __   *
//* \ \ /\ / / __|/ _` | / __| __/ _` | '__| __| | | | '_ \  *
//*  \ V  V /\__ \ (_| | \__ \ || (_| | |  | |_| |_| | |_) | *
//*   \_/\_/ |___/\__,_| |___/\__\__,_|_|   \__|\__,_| .__/  *
//*                                                  |_|     *
//===- lib/os/wsa_startup.cpp ---------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/os/wsa_startup.hpp"

#ifdef _WIN32
#    include <Winsock2.h>

namespace pstore {

    wsa_startup::~wsa_startup () noexcept {
        if (started_) {
            WSACleanup ();
        }
    }

    bool wsa_startup::start () noexcept {
        WSAData wsa_data;
        return WSAStartup (MAKEWORD (2, 2), &wsa_data) == 0;
    }

} // end namespace pstore

#endif // _WIN32
