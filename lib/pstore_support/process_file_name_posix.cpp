//*                                       __ _ _                                    *
//*  _ __  _ __ ___   ___ ___  ___ ___   / _(_) | ___   _ __   __ _ _ __ ___   ___  *
//* | '_ \| '__/ _ \ / __/ _ \/ __/ __| | |_| | |/ _ \ | '_ \ / _` | '_ ` _ \ / _ \ *
//* | |_) | | | (_) | (_|  __/\__ \__ \ |  _| | |  __/ | | | | (_| | | | | | |  __/ *
//* | .__/|_|  \___/ \___\___||___/___/ |_| |_|_|\___| |_| |_|\__,_|_| |_| |_|\___| *
//* |_|                                                                             *
//===- lib/pstore_support/process_file_name_posix.cpp ---------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
/// \file process_file_name_posix.cpp

#include "pstore_support/process_file_name.hpp"

#if !defined(_WIN32)

#include "pstore/config/config.hpp"
#include "pstore_support/error.hpp"
#include "pstore_support/portab.hpp"
#include "pstore_support/small_vector.hpp"

#if PSTORE_HAVE_NSGETEXECUTABLEPATH

// Include for _POSIX_PATH_MAX
#include <limits.h>      // NOLINT
#include <mach-o/dyld.h> // _NSGetExecutablePath

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
        assert (buffer_size > 1U);
        buffer[buffer_size - 1] = '\0'; // guarantee null termination.
        return {buffer.data ()};
    }

} // namespace pstore

#elif defined(__FreeBSD__)

// System-specific includes
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/param.h>

namespace pstore {

    std::string process_file_name () {
        std::array<int, 4> mib{{CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1}};
        small_vector<char> buffer;
        std::size_t const length =
            freebsd::process_file_name (gsl::make_span (mib), &::sysctl, buffer);
        return {buffer.data (), length};
    }

} // namespace pstore

#elif defined(__linux__)

// Standard includes
#include <climits>
#include <limits>
#include <sstream>

// OS-specific includes
#include <sys/types.h>
#include <unistd.h>

namespace {
    std::string link_path () {
        std::ostringstream link_stream;
        link_stream << "/proc/" << ::getpid () << "/exe";
        return link_stream.str ();
    }
} // namespace


namespace pstore {

    std::string process_file_name () {
        std::string path = link_path ();
        auto read_link = [&path](::pstore::gsl::span<char> buffer) {
            assert (buffer.size () <= SSIZE_MAX);
            ssize_t const num_chars = ::readlink (path.c_str (), buffer.data (), buffer.size ());
            if (num_chars < 0) {
                int const error = errno;
                std::ostringstream str;
                str << "readlink() of \"" << path << "\" failed";
                raise (errno_erc{error}, str.str ());
            }
            PSTORE_STATIC_ASSERT (std::numeric_limits<std::size_t>::max () >=
                                  std::numeric_limits<ssize_t>::max ());
            return static_cast<std::size_t> (num_chars);
        };

        small_vector<char> buffer;
        std::size_t const length = process_file_name (read_link, buffer);
        return {buffer.data (), length};
    }

} // namespace pstore

#endif // defined(__linux__)

#endif // !defined(_WIN32)
// eof: lib/pstore_support/process_file_name_posix.cpp
