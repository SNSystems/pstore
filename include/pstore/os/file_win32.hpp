//*   __ _ _       *
//*  / _(_) | ___  *
//* | |_| | |/ _ \ *
//* |  _| | |  __/ *
//* |_| |_|_|\___| *
//*                *
//===- include/pstore/os/file_win32.hpp -----------------------------------===//
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
