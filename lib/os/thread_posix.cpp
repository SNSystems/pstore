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

#include "pstore/os/thread.hpp"


#ifndef _WIN32

// Standard library includes
#    include <array>
#    include <cerrno>
#    include <cstring>
#    include <system_error>

// OS-specific includes
#    include <pthread.h>

#    ifdef __linux__
#        include <linux/unistd.h>
#        include <sys/syscall.h>
#        include <unistd.h>
#    endif
#    ifdef __FreeBSD__
#        include <pthread_np.h>
#    endif

// pstore includes
#    include "pstore/config/config.hpp"
#    include "pstore/support/error.hpp"

namespace pstore {
    namespace threads {

#    ifdef __FreeBSD__
        static thread_local char thread_name[name_size];
#    endif // __FreeBSD__

        void set_name (gsl::not_null<gsl::czstring> const name) {
            // pthread support for setting thread names comes in various non-portable forms.
            // Here I'm supporting four versions:
            // - the single argument version used by macOS.
            // - two argument form supported by Linux.
            // - three argument form supported by NetBSD.
            // - the slightly differently named form used in FreeBSD.
#    ifdef __FreeBSD__
            std::strncpy (thread_name, name, name_size);
            thread_name[name_size - 1] = '\0';
#    else
            int err = 0;
#        if defined(PSTORE_PTHREAD_SETNAME_NP_1_ARG)
            err = pthread_setname_np (name);
#        elif defined(PSTORE_PTHREAD_SETNAME_NP_2_ARGS)
            err = pthread_setname_np (pthread_self (), name);
#        elif defined(PSTORE_PTHREAD_SETNAME_NP_3_ARGS)
            err = pthread_setname_np (pthread_self (), name, nullptr);
#        elif defined(PSTORE_PTHREAD_SET_NAME_NP)
            pthread_set_name_np (pthread_self (), name);
#        else
#            error "pthread_setname was not available"
#        endif
            if (err != 0) {
                raise (errno_erc{err}, "pthread_set_name_np");
            }
#    endif // __FreeBSD__
        }

        gsl::czstring get_name (gsl::span<char, name_size> const name /*out*/) {
            auto const length = name.size ();
            if (name.data () == nullptr || length < 1) {
                raise (errno_erc{EINVAL});
            }
#    ifdef __FreeBSD__
            std::strncpy (name.data (), thread_name, static_cast<std::size_t> (length));
#    else
            PSTORE_ASSERT (length == name.size_bytes ());
            int const err = pthread_getname_np (pthread_self (), name.data (),
                                                static_cast<std::size_t> (length));
            if (err != 0) {
                raise (errno_erc{err}, "pthread_getname_np");
            }
#    endif // __FreeBSD__
            name[length - 1] = '\0';
            return name.data ();
        }

    } // end namespace threads
} // end namespace pstore

#endif // !_WIN32
