//*                       *
//*  ___  ___ __ _ _ __   *
//* / __|/ __/ _` | '_ \  *
//* \__ \ (_| (_| | | | | *
//* |___/\___\__,_|_| |_| *
//*                       *
//===- tools/genromfs/scan.cpp --------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
#include "pstore/support/quoted.hpp"
#include "pstore/support/utf.hpp"

#include "copy.hpp"
#include "directory_entry.hpp"

using namespace std::string_literals;

namespace {

    unsigned add_directory (directory_container & directory, std::string const & path_name,
                            std::string const & file_name, unsigned count) {
        directory.emplace_back (file_name, count, std::make_unique<directory_container> ());
        return scan (*directory.back ().children, path_name, count + 1U);
    }

    unsigned add_file (directory_container & directory, std::string const & path_name,
                       std::string const & file_name, unsigned count, std::time_t modtime) {
        directory.emplace_back (file_name, count, modtime);
        copy (path_name, directory.back ().contents);
        return count + 1U;
    }

    bool ends_with (std::string const & str, std::string const & suffix) {
        auto const suffix_size = suffix.size ();
        auto const str_size = str.size ();
        return str_size >= suffix_size &&
               str.compare (str_size - suffix_size, suffix_size, suffix) == 0;
    }

    bool skip_file (std::string const & name) {
        if (name.length () == 0 || name.front () == '.') {
            return true;
        }
        return ends_with (name, "~"s);
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
    auto is_hidden = [] (WIN32_FIND_DATA const & fd) -> bool {
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
        if (skip_file (name)) {
            continue;
        }
        auto const path_string = path + '/' + name;
        count = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    ? add_directory (directory, path_string, name, count)
                    : add_file (directory, path_string, name, count,
                                filetime_to_timet (ffd.ftLastWriteTime));
    } while (::FindNextFileW (find, &ffd) != 0);

    ::FindClose (find);
    std::sort (
        std::begin (directory), std::end (directory),
        [] (directory_entry const & a, directory_entry const & b) { return a.name < b.name; });
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

    while (struct dirent const * const dp = readdir (dirp.get ())) {
        auto const name = std::string{dp->d_name};
        if (skip_file (name)) {
            continue;
        }

        auto const path_string = path + '/' + name;
        struct stat sb {};
        if (lstat (path_string.c_str (), &sb) != 0) {
            auto last_error = errno;
            std::ostringstream str;
            str << "Could not stat file " << pstore::quoted (path_string);
            raise (pstore::errno_erc (last_error), str.str ());
        }

        // NOLINTNEXTLINE
        if (S_ISREG (sb.st_mode)) { //! OCLINT(PH - don't warn about system macro)
            // It's a regular file
            count = add_file (directory, path_string, name, count, sb.st_mtime);
        } else {
            // NOLINTNEXTLINE
            if (S_ISDIR (sb.st_mode)) { //! OCLINT(PH - don't warn about system macro)
                // A directory
                count = add_directory (directory, path_string, name, count);
            } else {
                // skip
            }
        }
    }

    std::sort (
        std::begin (directory), std::end (directory),
        [] (directory_entry const & a, directory_entry const & b) { return a.name < b.name; });
    return count;
}

#endif
