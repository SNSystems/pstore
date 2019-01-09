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

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define NO_MIN_MAX
#    include <Windows.h>
#else
#    include <dirent.h>
#    include <sys/stat.h>
#endif

#include "pstore/support/make_unique.hpp"
#include "pstore/support/utf.hpp"

#include "copy.hpp"
#include "directory_entry.hpp"

namespace {

    unsigned add_directory (directory_container & directory, std::string const & path,
                            std::string const & file_name, unsigned count) {
        directory.emplace_back (file_name, count, std::make_unique<directory_container> ());
        return scan (*directory.back ().children, path + '/' + file_name, count + 1U);
    }

    unsigned add_file (directory_container & directory, std::string const & path,
                       std::string const & file_name, unsigned count) {
        directory.emplace_back (file_name, count);
        copy (path + '/' + file_name, directory.back ().contents);
        return count + 1U;
    }

} // end anonymous namespace


#ifdef _WIN32

unsigned scan (directory_container & directory, std::string const & path, unsigned count) {
    auto is_hidden = [](WIN32_FIND_DATA const & fd) -> bool {
        return fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN;
    };

    WIN32_FIND_DATA ffd;
    HANDLE find = ::FindFirstFileW (pstore::utf::win32::to16 (path + "\\*").c_str (), &ffd);
    if (find == INVALID_HANDLE_VALUE) {
        // printf ("FindFirstFile failed (%d)\n", GetLastError ());
        return count;
    }

    do {
        if (!is_hidden (ffd)) {
            auto name = pstore::utf::win32::to8 (ffd.cFileName);
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                count = add_directory (directory, path, name, count);
            } else {
                count = add_file (directory, path, name, count);
            }
        }
    } while (::FindNextFileW (find, &ffd) != 0);

    ::FindClose (find);
    return count;
}

#else

unsigned scan (directory_container & directory, std::string const & path, unsigned count) {

    std::unique_ptr<DIR, decltype (&closedir)> dirp (opendir (path.c_str ()), &closedir);
    if (dirp == nullptr) {
        return count;
    }

    auto is_hidden = [](std::string const & n) { return n.length () == 0 || n.front () == '.'; };

    while (struct dirent const * dp = readdir (dirp.get ())) {
        auto const name = std::string (dp->d_name);
        if (!is_hidden (name)) {
            struct stat sb;
            if (lstat ((path + '/' + name).c_str (), &sb) != 0) {
                return count;
            }

            if (S_ISREG (sb.st_mode)) {
                // It's a regular file
                count = add_file (directory, path, name, count);
            } else if (S_ISDIR (sb.st_mode)) {
                // A directory
                count = add_directory (directory, path, name, count);
            } else {
                // skip
            }
        }
    }
    return count;
}

#endif
