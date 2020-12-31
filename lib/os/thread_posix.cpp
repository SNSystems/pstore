//*  _   _                        _  *
//* | |_| |__  _ __ ___  __ _  __| | *
//* | __| '_ \| '__/ _ \/ _` |/ _` | *
//* | |_| | | | | |  __/ (_| | (_| | *
//*  \__|_| |_|_|  \___|\__,_|\__,_| *
//*                                  *
//===- lib/os/thread_posix.cpp --------------------------------------------===//
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
