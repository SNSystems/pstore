//*                                  *
//*  ___ _ __   __ ___      ___ __   *
//* / __| '_ \ / _` \ \ /\ / / '_ \  *
//* \__ \ |_) | (_| |\ V  V /| | | | *
//* |___/ .__/ \__,_| \_/\_/ |_| |_| *
//*     |_|                          *
//===- include/pstore/broker/spawn.hpp ------------------------------------===//
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
/// \file spawn.hpp

#ifndef PSTORE_BROKER_SPAWN_HPP
#define PSTORE_BROKER_SPAWN_HPP

#include <functional>
#include <memory>
#include <string>
#include <type_traits>

// platform includes
#ifdef _WIN32
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#else
#    include <sys/types.h>
#endif // _WIN32

#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace broker {

#ifdef _WIN32
        namespace win32 {

            /// This routine appends the given argument to a command line such
            /// that CommandLineToArgvW will return the argument string unchanged.
            /// Arguments in a command line should be separated by spaces; this
            /// function does not add these spaces.
            ///
            /// Based on code published in an MSDN blog article titled "Everyone
            /// quotes command line arguments the wrong way" (Daniel Colascione,
            /// April 23 2011).
            ///
            /// \note This function is exposed to enable it to be unit tested.
            ///
            /// \param arg  The argument to encode.
            /// \param force  Supplies an indication of whether we should quote the argument even if
            /// it does not contain any characters that would ordinarily require quoting.
            /// \return  The quoted argument string.
            std::string argv_quote (std::string const & arg, bool force = false);

            /// Takes an array of command-line argument strings and converts them
            /// to a single string
            ///
            /// \note This function is exposed to enable it to be unit tested.
            std::string build_command_line (gsl::czstring * argv);

            struct process_pair {
                constexpr process_pair (HANDLE p, DWORD g) noexcept
                        : process{p}
                        , group{g} {}
                ~process_pair () noexcept {
                    if (process != nullptr) {
                        ::CloseHandle (process);
                    }
                }

                process_pair (process_pair const &) = delete;
                process_pair (process_pair &&) noexcept = delete;
                process_pair & operator= (process_pair const &) = delete;
                process_pair & operator= (process_pair &&) noexcept = delete;

                HANDLE const process;
                DWORD const group;
            };

        } // end namespace win32
#endif

#ifdef _WIN32
        using process_identifier = std::shared_ptr<win32::process_pair>;
#else
        using process_identifier = pid_t;
#endif

        /// Starts the process at the location given by `exe_path` and with the arguments
        /// supplied in a null-terminated array of czstrings.
        process_identifier spawn (gsl::czstring exe_path, gsl::czstring * argv);

    } // namespace broker
} // namespace pstore

#endif // PSTORE_BROKER_SPAWN_HPP
