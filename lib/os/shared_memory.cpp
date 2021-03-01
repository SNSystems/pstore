//===- lib/os/shared_memory.cpp -------------------------------------------===//
//*      _                        _                                              *
//*  ___| |__   __ _ _ __ ___  __| |  _ __ ___   ___ _ __ ___   ___  _ __ _   _  *
//* / __| '_ \ / _` | '__/ _ \/ _` | | '_ ` _ \ / _ \ '_ ` _ \ / _ \| '__| | | | *
//* \__ \ | | | (_| | | |  __/ (_| | | | | | | |  __/ | | | | | (_) | |  | |_| | *
//* |___/_| |_|\__,_|_|  \___|\__,_| |_| |_| |_|\___|_| |_| |_|\___/|_|   \__, | *
//*                                                                       |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file shared_memory.cpp

#include "pstore/os/shared_memory.hpp"

#include "pstore/config/config.hpp"

#ifdef PSTORE_HAVE_SYS_POSIX_SHM_H
#    include <sys/posix_shm.h>
#    include <sys/time.h>
#    include <sys/types.h>
#endif // PSTORE_HAVE_SYS_POSIX_SHM_H

#if !defined(PSHMNAMLEN) && defined(PSTORE_HAVE_LINUX_LIMITS_H)
#    include <linux/limits.h>
#    if !defined(PSHMNAMLEN) && defined(NAME_MAX)
#        define PSHMNAMLEN NAME_MAX
#    endif
#endif // PSTORE_HAVE_LINUX_LIMITS_H

// Just in case no one has defined... we'll name up a reasonable value.
#ifndef PSHMNAMLEN
#    ifdef __APPLE__
#        define PSHMNAMLEN (31) // taken from bsd/kern/posix_shm.c
#    else
#        define PSHMNAMLEN (100)
#    endif
#endif // PSHMNAMLEN


namespace pstore {

    std::size_t get_pshmnamlen () noexcept {
#ifdef _WIN32
        return MAX_PATH;
#else
        return PSHMNAMLEN;
#endif
    }

} // namespace pstore
