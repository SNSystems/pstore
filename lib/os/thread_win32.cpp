//===- lib/os/thread_win32.cpp --------------------------------------------===//
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
/// \file thread.cpp

#include "pstore/os/thread.hpp"

#ifdef _WIN32

#    include <array>
#    include <cerrno>
#    include <cstring>
#    include <system_error>

#    include "pstore/config/config.hpp"
#    include "pstore/support/error.hpp"

namespace pstore {
    namespace threads {

        static thread_local char thread_name[name_size];

        void set_name (gsl::not_null<gsl::czstring> name) {
            // This code taken from http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
            // Sadly, threads don't actually have names in Win32. The process via
            // RaiseException is just a "Secret Handshake" with the VS Debugger, who
            // actually stores the thread -> name mapping. Windows itself has no
            // notion of a thread "name".

            std::strncpy (thread_name, name, name_size);
            thread_name[name_size - 1] = '\0';
#    ifndef NDEBUG
            DWORD const MS_VC_EXCEPTION = 0x406D1388;

#        pragma pack(push, 8)
            struct THREADNAME_INFO {
                DWORD dwType;     // Must be 0x1000.
                LPCSTR szName;    // Pointer to name (in user addr space).
                DWORD dwThreadID; // Thread ID (-1=caller thread).
                DWORD dwFlags;    // Reserved for future use, must be zero.
            };
#        pragma pack(pop)

            THREADNAME_INFO info;
            info.dwType = 0x1000;
            info.szName = name;
            info.dwThreadID = GetCurrentThreadId ();
            info.dwFlags = 0;

            __try {
                RaiseException (MS_VC_EXCEPTION, 0, sizeof (info) / sizeof (ULONG_PTR),
                                (ULONG_PTR *) &info);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
            }
#    endif // NDEBUG
        }

        gsl::czstring get_name (gsl::span<char, name_size> name /*out*/) {
            auto const length = name.size ();
            if (name.data () == nullptr || length < 1) {
                raise (errno_erc{EINVAL});
            }
            std::strncpy (name.data (), thread_name, length);
            name[length - 1] = '\0';
            return name.data ();
        }

    } // end namespace threads
} // end namespace pstore

#endif //_WIN32
