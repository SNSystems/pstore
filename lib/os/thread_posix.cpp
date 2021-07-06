//===- lib/os/thread_posix.cpp --------------------------------------------===//
//*  _   _                        _  *
//* | |_| |__  _ __ ___  __ _  __| | *
//* | __| '_ \| '__/ _ \/ _` |/ _` | *
//* | |_| | | | | |  __/ (_| | (_| | *
//*  \__|_| |_|_|  \___|\__,_|\__,_| *
//*                                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file thread_posix.cpp
/// \brief  Implementation of functions to get and set the name of the current thread on POSIX
///   systems.

#include "pstore/os/thread.hpp"

#ifndef _WIN32

// Standard library includes
#    include <array>
#    include <cerrno>
#    include <cstring>
#    include <system_error>

// OS-specific includes
#    include <pthread.h>

#    ifdef PSTORE_HAVE_LINUX_UNISTD_H
#        include <linux/unistd.h>
#    endif
#    ifdef PSTORE_HAVE_SYS_SYSCALL_H
#        include <sys/syscall.h>
#    endif
#    include <unistd.h>

#    ifdef __FreeBSD__
#        include <pthread_np.h>
#    endif

// pstore includes
#    include "pstore/config/config.hpp"
#    include "pstore/support/error.hpp"
#    include "pstore/support/unsigned_cast.hpp"

namespace pstore {
    namespace threads {
#    if defined(PSTORE_PTHREAD_SETNAME_NP_1_ARG) || defined(PSTORE_PTHREAD_SETNAME_NP_2_ARGS) ||   \
        defined(PSTORE_PTHREAD_SETNAME_NP_3_ARGS) || defined(PSTORE_PTHREAD_SET_NAME_NP)
#        define PTHREAD_SETNAME_NP 1
#    endif

#    if defined(PSTORE_PTHREAD_GETNAME_NP) || defined(PSTORE_PTHREAD_GET_NAME_NP)
#        define PTHREAD_GETNAME_NP 1
#    endif

#    if !defined(PTHREAD_SETNAME_NP) && !defined(PTHREAD_GETNAME_NP)
#        define USE_FALLBACK 1
#    endif

#    ifdef USE_FALLBACK
        static thread_local char thread_name[name_size];
#    endif

        void set_name (gsl::not_null<gsl::czstring> const name) {
            // pthread support for setting thread names comes in various non-portable forms.
            // Here I'm supporting four versions:
            // - the single argument version used by macOS.
            // - two argument form supported by Linux.
            // - three argument form supported by NetBSD.
            // - the slightly differently named form used in FreeBSD.
            int err = 0;
#    ifdef USE_FALLBACK
            std::strncpy (thread_name, name, name_size);
            thread_name[name_size - 1] = '\0';
#    elif defined(PSTORE_PTHREAD_SETNAME_NP_1_ARG)
            err = pthread_setname_np (name);
#    elif defined(PSTORE_PTHREAD_SETNAME_NP_2_ARGS)
            err = pthread_setname_np (pthread_self (), name);
#    elif defined(PSTORE_PTHREAD_SETNAME_NP_3_ARGS)
            err = pthread_setname_np (pthread_self (), name, nullptr);
#    elif defined(PSTORE_PTHREAD_SET_NAME_NP)
            pthread_set_name_np (pthread_self (), name);
#    else
#        error "Don't know how to set the name of the current thread"
#    endif
            if (err != 0) {
                raise (errno_erc{err}, "threads::set_name");
            }
        }

        gsl::czstring get_name (gsl::span<char, name_size> const name /*out*/) {
            auto const length = name.size ();
            if (name.data () == nullptr || length < 1) {
                raise (errno_erc{EINVAL});
            }

            PSTORE_ASSERT (length == name.size_bytes ());
            auto const length_u = unsigned_cast (length);
            int err = 0;
#    ifdef USE_FALLBACK
            std::strncpy (name.data (), thread_name, length_u);
#    elif defined(PSTORE_PTHREAD_GETNAME_NP)
            err = pthread_getname_np (pthread_self (), name.data (), length_u);
#    elif defined(PSTORE_PTHREAD_GET_NAME_NP)
            pthread_get_name_np (pthread_self (), name.data (), length_u);
#    else
#        error "Don't know how to get the name of the current thread"
#    endif
            if (err != 0) {
                raise (errno_erc{err}, "threads::get_name");
            }
            name[length - 1] = '\0';
            return name.data ();
        }

    } // end namespace threads
} // end namespace pstore

#endif // !_WIN32
