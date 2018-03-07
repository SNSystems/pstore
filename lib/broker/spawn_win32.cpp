//*                                  *
//*  ___ _ __   __ ___      ___ __   *
//* / __| '_ \ / _` \ \ /\ / / '_ \  *
//* \__ \ |_) | (_| |\ V  V /| | | | *
//* |___/ .__/ \__,_| \_/\_/ |_| |_| *
//*     |_|                          *
//===- lib/broker/spawn_win32.cpp -----------------------------------------===//
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
/// \file spawn_win32.cpp

#include "broker/spawn.hpp"

#ifdef _WIN32

// standard includes
#include <cstring>

// platform includes
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// pstore includes
#include "pstore_broker_intf/descriptor.hpp"
#include "pstore_support/error.hpp"
#include "pstore_support/utf.hpp"


namespace pstore {
    namespace broker {
        namespace win32 {

            std::string argv_quote (std::string const & in_arg, bool force) {
                // Unless we're told otherwise, don't quote unless we actually
                // need to do so. That will avoid problems if programs don't
                // parse quotes properly

                if (!force && !in_arg.empty () &&
                    in_arg.find_first_of (" \t\n\v\"\\") == in_arg.npos) {
                    return in_arg;
                }

                std::string res;
                res.reserve (in_arg.length () + 2);
                res.push_back ('"');

                for (auto it = std::begin (in_arg), end = std::end (in_arg); it != end; ++it) {
                    auto num_backslashes = 0U;

                    // Count the number of sequential backslashes.
                    while (it != end && *it == '\\') {
                        ++it;
                        ++num_backslashes;
                    }

                    if (it == end) {
                        // Escape all backslashes, but let the terminating
                        // double quotation mark we add below be interpreted
                        // as a metacharacter.
                        res.append (num_backslashes * 2U, '\\');
                        break;
                    }

                    if (*it == '"') {
                        // Escape all backslashes and the following
                        // double quotation mark.
                        res.append (num_backslashes * 2U + 1U, '\\');
                    } else {
                        // Backslashes aren't special here.
                        res.append (num_backslashes, '\\');
                    }
                    res.push_back (*it);
                }

                res.push_back ('"');
                return res;
            }

            std::string build_command_line (::pstore::gsl::czstring * argv) {
                std::string command_line;
                auto separator = "";
                for (char const ** arg = argv; *arg != nullptr; ++arg) {
                    command_line.append (separator);
                    command_line.append (argv_quote (*arg, false));
                    separator = " ";
                }
                return command_line;
            }

        } // namespace win32

        process_identifier spawn (::pstore::gsl::czstring exe_path,
                                  ::pstore::gsl::czstring * argv) {
            auto const exe_path_utf16 = pstore::utf::win32::to16 (exe_path);
            auto const command_line = pstore::utf::win32::to16 (win32::build_command_line (argv));

            STARTUPINFOW startup_info;
            std::memset (&startup_info, 0, sizeof (startup_info));
            startup_info.cb = sizeof (startup_info);

            PROCESS_INFORMATION process_information;
            std::memset (&process_information, 0, sizeof (process_information));

            if (!::CreateProcessW (
                    const_cast<wchar_t *> (exe_path_utf16.c_str ()),
                    const_cast<wchar_t *> (command_line.c_str ()),
                    static_cast<SECURITY_ATTRIBUTES *> (nullptr), // process attributes
                    static_cast<SECURITY_ATTRIBUTES *> (nullptr), // thread attributes
                    false,                                        // inherit handles
                    BELOW_NORMAL_PRIORITY_CLASS,                  // creation flags
                    nullptr,                                      // environment
                    nullptr, // working directory for the new process
                    &startup_info, &process_information)) {

                raise (::pstore::win32_erc (::GetLastError ()), "CreateProcessW");
            }

            // Close the main-thread handle. The process handle is needed to provide a robust
            // reference
            // to the process later on.
            ::CloseHandle (process_information.hThread);

            return std::shared_ptr<void> (process_information.hProcess, ::CloseHandle);
        }

    } // namespace broker
} // namespace pstore

#endif //_WIN32
// eof: lib/broker/spawn_win32.cpp
