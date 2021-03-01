//===- include/pstore/os/file_win32.hpp -------------------*- mode: C++ -*-===//
//*   __ _ _       *
//*  / _(_) | ___  *
//* | |_| | |/ _ \ *
//* |  _| | |  __/ *
//* |_| |_|_|\___| *
//*                *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file file_win32.hpp
/// \brief Windows-specific implementations of file APIs.

#ifndef PSTORE_OS_FILE_WIN32_HPP
#define PSTORE_OS_FILE_WIN32_HPP

#ifdef _WIN32

#    include <algorithm>
#    include <cassert>
#    include <cstdlib>
#    include <limits>

#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>

#    include "pstore/support/utf.hpp"

namespace pstore {
    namespace file {

        /// \brief A namespace to hold Win32-specific file interfaces.
        namespace win32 {
            class deleter final : public deleter_base {
            public:
                explicit deleter (std::string const & path)
                        : deleter_base (path, &platform_unlink) {}
                ~deleter () noexcept override;

            private:
                /// The platform-specific file deletion function. file_deleter_base will
                /// call this function when it wants to delete a file.
                /// \param path The UTF-8 encoded path to the file to be deleted.
                static void platform_unlink (std::string const & path);
            };
        } // namespace win32

        /// \brief The cross-platform name for the deleter class.
        /// This should always be preferred to the platform-specific variation.
        using deleter = win32::deleter;
    } // namespace file
} // namespace pstore

#endif //_WIN32
#endif // PSTORE_OS_FILE_WIN32_HPP
