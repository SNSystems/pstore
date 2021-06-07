//===- lib/os/process_file_name_posix.cpp ---------------------------------===//
//*                                       __ _ _                                    *
//*  _ __  _ __ ___   ___ ___  ___ ___   / _(_) | ___   _ __   __ _ _ __ ___   ___  *
//* | '_ \| '__/ _ \ / __/ _ \/ __/ __| | |_| | |/ _ \ | '_ \ / _` | '_ ` _ \ / _ \ *
//* | |_) | | | (_) | (_|  __/\__ \__ \ |  _| | |  __/ | | | | (_| | | | | | |  __/ *
//* | .__/|_|  \___/ \___\___||___/___/ |_| |_|_|\___| |_| |_|\__,_|_| |_| |_|\___| *
//* |_|                                                                             *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file process_file_name_posix.cpp

#include "pstore/os/process_file_name.hpp"

#if !defined(_WIN32)

#    include "pstore/config/config.hpp"
#    include "pstore/adt/small_vector.hpp"
#    include "pstore/support/error.hpp"
#    include "pstore/support/portab.hpp"
#    include "pstore/support/unsigned_cast.hpp"

#    ifdef PSTORE_HAVE_NSGETEXECUTABLEPATH

// Include for _POSIX_PATH_MAX
#        include <limits.h>      // NOLINT
#        include <mach-o/dyld.h> // _NSGetExecutablePath

namespace pstore {

    std::string process_file_name () {
        small_vector<char, _POSIX_PATH_MAX> buffer (std::size_t{_POSIX_PATH_MAX});
        auto buffer_size = static_cast<std::uint32_t> (buffer.size ());
        int resl = ::_NSGetExecutablePath (buffer.data (), &buffer_size);
        if (resl == -1) {
            // Note that _NSGetExecutablePath will modify buffer_size to tell us the correct amount
            // of storage to allocate.
            buffer.resize (buffer_size);
            ::_NSGetExecutablePath (buffer.data (), &buffer_size);
        }
        PSTORE_ASSERT (buffer_size > 1U);
        buffer[buffer_size - 1] = '\0'; // guarantee null termination.
        return {buffer.data ()};
    }

} // end namespace pstore

#    elif defined(__FreeBSD__)

// System-specific includes
#        include <sys/types.h>
#        include <sys/sysctl.h>
#        include <sys/param.h>

namespace pstore {

    std::string process_file_name () {
        std::array<int, 4> mib{{CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1}};
        small_vector<char> buffer;
        std::size_t const length =
            freebsd::process_file_name (gsl::make_span (mib), &::sysctl, buffer);
        return {buffer.data (), length};
    }

} // end namespace pstore

#    elif defined(__linux__) || defined(__sun__) || defined(__NetBSD__)

// Standard includes
#        include <climits>
#        include <limits>
#        include <sstream>

// OS-specific includes
#        include <sys/types.h>
#        include <unistd.h>

namespace {

#        ifdef __linux__
    std::string link_path () {
        std::ostringstream link_stream;
        link_stream << "/proc/" << ::getpid () << "/exe";
        return link_stream.str ();
    }
#        elif defined(__sun__)
    std::string link_path () {
        std::ostringstream link_stream;
        link_stream << "/proc/" << ::getpid () << "/path/a.out";
        return link_stream.str ();
    }
#        elif defined(__NetBSD__)
    std::string link_path () { return "/proc/curproc/exe"; }
#        else
#            error "Don't know how to build link path for this system"
#        endif

    template <typename T>
    T clamp (T v, T low, T high) {
        return std::min (std::max (v, low), high);
    }

} // end anonymous namespace


namespace pstore {

    std::string process_file_name () {
        std::string const path = link_path ();
        auto read_link = [&path] (gsl::span<char> const buffer) {
            auto const buffer_size = buffer.size ();
            PSTORE_ASSERT (buffer_size >= 0);
            ssize_t const num_chars = ::readlink (path.c_str (), buffer.data (),
                                                  clamp (static_cast<std::size_t> (buffer_size),
                                                         std::size_t{0}, std::size_t{SSIZE_MAX}));
            if (num_chars < 0) {
                int const error = errno;
                std::ostringstream str;
                str << "readlink() of \"" << path << "\" failed";
                raise (errno_erc{error}, str.str ());
            }
            PSTORE_STATIC_ASSERT (std::numeric_limits<std::size_t>::max () >=
                                  unsigned_cast (std::numeric_limits<ssize_t>::max ()));
            return static_cast<std::size_t> (num_chars);
        };

        small_vector<char> buffer;
        std::size_t const length = process_file_name (read_link, buffer);
        return {buffer.data (), length};
    }

} // end namespace pstore

#    else // defined(__linux__)
#        error "Don't know how to implement process_file_name() on this system"
#    endif

#endif // !defined(_WIN32)
