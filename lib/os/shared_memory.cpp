//*      _                        _                                              *
//*  ___| |__   __ _ _ __ ___  __| |  _ __ ___   ___ _ __ ___   ___  _ __ _   _  *
//* / __| '_ \ / _` | '__/ _ \/ _` | | '_ ` _ \ / _ \ '_ ` _ \ / _ \| '__| | | | *
//* \__ \ | | | (_| | | |  __/ (_| | | | | | | |  __/ | | | | | (_) | |  | |_| | *
//* |___/_| |_|\__,_|_|  \___|\__,_| |_| |_| |_|\___|_| |_| |_|\___/|_|   \__, | *
//*                                                                       |___/  *
//===- lib/os/shared_memory.cpp -------------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
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
