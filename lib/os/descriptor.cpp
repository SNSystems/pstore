//===- lib/os/descriptor.cpp ----------------------------------------------===//
//*      _                     _       _              *
//*   __| | ___  ___  ___ _ __(_)_ __ | |_ ___  _ __  *
//*  / _` |/ _ \/ __|/ __| '__| | '_ \| __/ _ \| '__| *
//* | (_| |  __/\__ \ (__| |  | | |_) | || (_) | |    *
//*  \__,_|\___||___/\___|_|  |_| .__/ \__\___/|_|    *
//*                             |_|                   *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/os/descriptor.hpp"

#if defined(_WIN32) && defined(_MSC_VER)

namespace pstore {
    namespace details {

        win32_pipe_descriptor_traits::type const win32_pipe_descriptor_traits::invalid =
            INVALID_HANDLE_VALUE;
        win32_pipe_descriptor_traits::type const win32_pipe_descriptor_traits::error =
            INVALID_HANDLE_VALUE;

    } // end namespace details
} // end namespace pstore

#endif // defined(_WIN32) && defined(_MSC_VER)
