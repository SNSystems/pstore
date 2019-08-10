//*              _   _      *
//*  _ __   __ _| |_| |__   *
//* | '_ \ / _` | __| '_ \  *
//* | |_) | (_| | |_| | | | *
//* | .__/ \__,_|\__|_| |_| *
//* |_|                     *
//===- include/pstore/support/path.hpp ------------------------------------===//
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
/// \file path.hpp
/// \brief Functions to operate on native file paths.

#ifndef PSTORE_SUPPORT_PATH_HPP
#define PSTORE_SUPPORT_PATH_HPP

#include <initializer_list>
#include <string>
#include <utility>

#if defined(_WIN32)
#    define PSTORE_PLATFORM_NS win32
#else
#    define PSTORE_PLATFORM_NS posix
#endif

namespace pstore {
    /// \brief The 'path' namespace contains all of the functions that are used to manipulated host
    /// file paths.
    namespace path {

        /// \brief POSIX-specific path name handling.
        namespace posix {
            std::pair<std::string, std::string> split_drive (std::string const & path);
            std::string dir_name (std::string const & path);
            std::string base_name (std::string const & path);

            std::string join (std::string const & path,
                              std::initializer_list<std::string> const & paths);
            inline std::string join (std::string const & path, std::string const & b) {
                return join (path, {b});
            }
        } // namespace posix

        /// \brief Win32-specific path name handling.
        namespace win32 {
            std::pair<std::string, std::string> split_drive (std::string const & path);
            std::string dir_name (std::string const & path);
            std::string base_name (std::string const & path);

            std::string join (std::string const & path,
                              std::initializer_list<std::string> const & paths);
            inline std::string join (std::string const & path, std::string const & b) {
                return join (path, {b});
            }
        } // namespace win32

        inline std::pair<std::string, std::string> split_drive (std::string const & path) {
            return PSTORE_PLATFORM_NS::split_drive (path);
        }

        inline std::string base_name (std::string const & path) {
            return PSTORE_PLATFORM_NS::base_name (path);
        }

        inline std::string dir_name (std::string const & path) {
            return PSTORE_PLATFORM_NS::dir_name (path);
        }


        /// \name Path joining functions
        /// Join one or more path components intelligently. The return value is the concatenation of
        /// path and any members of *paths with exactly one directory separator following
        /// each non-empty part except the last, meaning that the result will only end in a
        /// separator if the last part is empty. If a component is an absolute path, all previous
        /// components are thrown away and joining continues from the absolute path component.
        ///
        /// On Windows, the drive letter is not reset when an absolute path component (e.g.,
        /// "\\foo") is encountered. If a component contains a drive letter, all previous components

        inline std::string join (std::string const & path,
                                 std::initializer_list<std::string> const & paths) {
            return PSTORE_PLATFORM_NS::join (path, paths);
        }

        inline std::string join (std::string const & path, std::string const & b) {
            return PSTORE_PLATFORM_NS::join (path, b);
        }
        ///@}


    } // namespace path
} // namespace pstore

#undef PSTORE_PLATFORM_NS

#endif // PSTORE_SUPPORT_PATH_HPP
