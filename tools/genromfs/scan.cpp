//*                       *
//*  ___  ___ __ _ _ __   *
//* / __|/ __/ _` | '_ \  *
//* \__ \ (_| (_| | | | | *
//* |___/\___\__,_|_| |_| *
//*                       *
//===- tools/genromfs/scan.cpp --------------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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

#include "scan.hpp"

#include <algorithm>
#include <cerrno>
#include <sstream>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define NO_MIN_MAX
#    include <Windows.h>
#else
#    include <dirent.h>
#    include <sys/stat.h>
#endif

#include "pstore/support/error.hpp"
#include "pstore/support/make_unique.hpp"
#include "pstore/support/quoted_string.hpp"
#include "pstore/support/utf.hpp"

#include "copy.hpp"
#include "directory_entry.hpp"

namespace {

    unsigned add_directory (directory_container & directory, std::string const & path,
                            std::string const & file_name, unsigned count) {
        directory.emplace_back (file_name, count, pstore::make_unique<directory_container> ());
        return scan (*directory.back ().children, path + '/' + file_name, count + 1U);
    }

    unsigned add_file (directory_container & directory, std::string const & path,
                       std::string const & file_name, unsigned count, std::time_t modtime) {
        directory.emplace_back (file_name, count, modtime);
        copy (path + '/' + file_name, directory.back ().contents);
        return count + 1U;
    }

} // end anonymous namespace


#ifdef _WIN32

namespace {

    std::time_t filetime_to_timet (FILETIME const & ft) noexcept {
        // Windows ticks are 100 nanoseconds.
        static constexpr auto windows_tick = 10000000ULL;
        // The number of seconds between the Windows epoch (1601-01-01T00:00:00Z) and the Unix epoch
        // (1601-01-01T00:00:00Z).
        static constexpr auto secs_to_unix_epoch = 11644473600ULL;

        ULARGE_INTEGER ull;
        ull.LowPart = ft.dwLowDateTime;
        ull.HighPart = ft.dwHighDateTime;
        return std::max (ull.QuadPart / windows_tick, secs_to_unix_epoch) - secs_to_unix_epoch;
    }

} // end anonymous namespace


unsigned scan (directory_container & directory, std::string const & path, unsigned count) {
    auto is_hidden = [](WIN32_FIND_DATA const & fd) -> bool {
        return fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN;
    };

    WIN32_FIND_DATA ffd;
    HANDLE find = ::FindFirstFileW (pstore::utf::win32::to16 (path + "\\*").c_str (), &ffd);
    if (find == INVALID_HANDLE_VALUE) {
        DWORD const last_error = ::GetLastError ();
        std::ostringstream str;
        str << "Could not scan directory " << pstore::quoted (path);
        raise (pstore::win32_erc (last_error), str.str ());
    }

    do {
        if (is_hidden (ffd)) {
            continue;
        }

        auto const name = pstore::utf::win32::to8 (ffd.cFileName);
        if (name == "." || name == "..") {
            continue;
        }

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            count = add_directory (directory, path, name, count);
        } else {
            ;
            count =
                add_file (directory, path, name, count, filetime_to_timet (ffd.ftLastWriteTime));
        }
    } while (::FindNextFileW (find, &ffd) != 0);

    ::FindClose (find);
    std::sort (
        std::begin (directory), std::end (directory),
        [](directory_entry const & a, directory_entry const & b) { return a.name < b.name; });
    return count;
}

#else

unsigned scan (directory_container & directory, std::string const & path, unsigned count) {
    std::unique_ptr<DIR, decltype (&closedir)> dirp (opendir (path.c_str ()), &closedir);
    if (dirp == nullptr) {
        int const erc = errno;
        std::ostringstream str;
        str << "opendir " << pstore::quoted (path);
        raise (pstore::errno_erc{erc}, str.str ());
    }

    auto is_hidden = [](std::string const & n) { return n.length () == 0 || n.front () == '.'; };
    auto path_string = [&path](std::string const & n) {
        std::string res = path;
        res += '/';
        res += n;
        return res;
    };

    while (struct dirent const * const dp = readdir (dirp.get ())) {
        auto const name = std::string (dp->d_name);
        if (is_hidden (name)) {
            continue;
        }

        struct stat sb {};
        if (lstat (path_string (name).c_str (), &sb) != 0) {
            return count;
        }

        // NOLINTNEXTLINE
        if (S_ISREG (sb.st_mode)) { //! OCLINT(PH - don't warn about system macro)
            // It's a regular file
            count = add_file (directory, path, name, count, sb.st_mtime);
        } else {
            // NOLINTNEXTLINE
            if (S_ISDIR (sb.st_mode)) { //! OCLINT(PH - don't warn about system macro)
                // A directory
                count = add_directory (directory, path, name, count);
            } else {
                // skip
            }
        }
    }

    std::sort (
        std::begin (directory), std::end (directory),
        [](directory_entry const & a, directory_entry const & b) { return a.name < b.name; });
    return count;
}

#endif
