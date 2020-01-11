//*              _   _      *
//*  _ __   __ _| |_| |__   *
//* | '_ \ / _` | __| '_ \  *
//* | |_) | (_| | |_| | | | *
//* | .__/ \__,_|\__|_| |_| *
//* |_|                     *
//===- lib/support/path.cpp -----------------------------------------------===//
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
/// \file path.cpp
#include "pstore/support/path.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iterator>
#include <tuple>

namespace {

    inline bool starts_with (std::string const & s, char const c) {
        return !s.empty () && s.front () == c;
    }
    inline bool ends_with (std::string const & s, char const c) {
        return !s.empty () && s.back () == c;
    }

} // end anonymous namespace

namespace pstore {
    namespace path {
        namespace posix {

            std::pair<std::string, std::string> split_drive (std::string const & p) {
                return {"", p};
            }

            std::string join (std::string const & a, std::initializer_list<std::string> const & p) {
                std::string path = a;
                for (auto const & b : p) {
                    if (starts_with (b, '/')) {
                        path = b;
                    } else if (path.empty () || ends_with (path, '/')) {
                        path += b;
                    } else {
                        path += '/';
                        path += b;
                    }
                }
                return path;
            }

        } // end namespace posix

        namespace win32 {

            // Split a path in a drive specification (a drive letter followed by a
            // colon) and the path specification.
            // It is always true that drivespec + pathspec == path
            //
            // If the path contained a drive letter, drivespec will contain everything
            // up to and including the colon.  e.g. split_drive("c:/dir") returns ("c:", "/dir")
            //
            // If the path contained a UNC path, the drivespec will contain the host name
            // and share up to but not including the fourth directory separator character.
            // e.g. split_drive("//host/computer/dir") returns ("//host/computer", "/dir")
            //
            // Paths cannot contain both a drive letter and a UNC path.

            std::pair<std::string, std::string> split_drive (std::string const & path) {
                if (path.length () <= 1) {
                    return {"", path};
                }

                // Normalize the slash direction.
                std::string normp;
                normp.reserve (path.length ());
                std::transform (std::begin (path), std::end (path), std::back_inserter (normp),
                                [] (char const c) { return c == '\\' ? '/' : c; });

                if ((normp.length () > 2 && normp[0] == '/' && normp[1] == '/') &&
                    (normp.length () > 3 && normp[2] != '/')) {
                    // This is is a UNC path:
                    // vvvvvvvvvvvvvvvvvvvv drive letter or UNC path
                    // \\machine\mountpoint\directory\etc\...
                    //           directory ^^^^^^^^^^^^^^^
                    std::string::size_type const index = normp.find ('/', 2);
                    if (index == std::string::npos) {
                        return {"", path};
                    }

                    std::string::size_type index2 = normp.find ('/', index + 1);
                    // An UNC path can't have two slashes in a row (after the initial two)
                    if (index2 == index + 1) {
                        return {"", path};
                    }
                    if (index2 == std::string::npos) {
                        index2 = path.length ();
                    }
                    return {path.substr (0, index2), path.substr (index2, std::string::npos)};
                }
                if (normp.length () > 1 && normp[1] == ':') {
                    return {path.substr (0, 2), path.substr (2, std::string::npos)};
                }
                return {"", path};
            }


            std::string join (std::string const & path,
                              std::initializer_list<std::string> const & paths) {
                auto is_path_sep = [] (char const c) { return c == '/' || c == '\\'; };

                std::string result_drive;
                std::string result_path;
                std::tie (result_drive, result_path) = split_drive (path);

                for (auto const & p : paths) {
                    std::string p_drive;
                    std::string p_path;
                    std::tie (p_drive, p_path) = split_drive (p);

                    if (!p_path.empty () && is_path_sep (p_path[0])) {
                        // The second path is absolute
                        if (!p_drive.empty () || result_drive.empty ()) {
                            result_drive = p_drive;
                        }

                        result_path = p_path;
                        continue;
                    }

                    if (!p_drive.empty () && p_drive != result_drive) {
                        assert (p_drive.length () == 2 && p_drive[1] == ':');
                        assert (result_drive.length () == 0 ||
                                (result_drive.length () == 2 && result_drive[1] == ':'));

                        if (result_drive.length () == 0 ||
                            std::tolower (p_drive[0]) != std::tolower (result_drive[0])) {
                            // Different drives so ignore the first path entirely
                            result_drive = p_drive;
                            result_path = p_path;
                            continue;
                        }

                        // Same drive in different case
                        result_drive = p_drive;
                    }

                    // Second path is relative to the first
                    if (!result_path.empty () && !is_path_sep (result_path.back ())) {
                        result_path += '\\';
                    }
                    result_path += p_path;
                }

                // Add separator between UNC and non-absolute path
                if (!result_path.empty () && !is_path_sep (result_path[0]) &&
                    !result_drive.empty () && result_drive.back () != ':') {

                    return result_drive + '\\' + result_path;
                }

                return result_drive + result_path;
            }

        } // end namespace win32

        namespace posix {

            std::string dir_name (std::string const & path) {
                std::string::size_type const pos = path.find_last_of ('/');
                return path.substr (0, (pos == std::string::npos) ? 0 : pos + 1);
            }

            std::string base_name (std::string const & path) {
                std::string::size_type const pos = path.find_last_of ('/');
                return path.substr ((pos == std::string::npos) ? 0 : pos + 1);
            }

        } // end namespace posix

        namespace win32 {

            std::string dir_name (std::string const & src_path) {
                std::string drive;
                std::string p;
                std::tie (drive, p) = path::win32::split_drive (src_path);

                std::string::size_type const pos = p.find_last_of (R"(/\)");
                p = p.substr (0, (pos == std::string::npos) ? 0 : pos + 1);
                return path::win32::join (drive, p);
            }

            std::string base_name (std::string const & path) {
                std::string drive;
                std::string p;
                std::tie (drive, p) = path::win32::split_drive (path);

                std::string::size_type const pos = p.find_last_of (R"(/\)");
                return p.substr ((pos == std::string::npos) ? 0 : pos + 1);
            }
        } // end namespace win32
    }     // end namespace path
} // end namespace pstore
