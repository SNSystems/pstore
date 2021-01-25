//*              _   _      *
//*  _ __   __ _| |_| |__   *
//* | '_ \ / _` | __| '_ \  *
//* | |_) | (_| | |_| | | | *
//* | .__/ \__,_|\__|_| |_| *
//* |_|                     *
//===- include/pstore/os/path.hpp -----------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file path.hpp
/// \brief Functions to operate on native file paths.

#ifndef PSTORE_OS_PATH_HPP
#define PSTORE_OS_PATH_HPP

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
        ///@{

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

#endif // PSTORE_OS_PATH_HPP
