//*  _   _                        _  *
//* | |_| |__  _ __ ___  __ _  __| | *
//* | __| '_ \| '__/ _ \/ _` |/ _` | *
//* | |_| | | | | |  __/ (_| | (_| | *
//*  \__|_| |_|_|  \___|\__,_|\__,_| *
//*                                  *
//===- include/pstore/os/thread.hpp ---------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file thread.hpp

#ifndef PSTORE_OS_THREAD_HPP
#define PSTORE_OS_THREAD_HPP

#include <cstdint>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#endif // _WIN32

#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace threads {

        constexpr std::size_t name_size = 16;
        void set_name (gsl::not_null<gsl::czstring> name);
        gsl::czstring get_name (gsl::span<char, name_size> name /*out*/);
        std::string get_name ();

    } // end namespace threads
} // end namespace pstore

#endif // PSTORE_OS_THREAD_HPP
