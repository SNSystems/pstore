//*   __ _  __                     _   _      *
//*  / _(_)/ _| ___    _ __   __ _| |_| |__   *
//* | |_| | |_ / _ \  | '_ \ / _` | __| '_ \  *
//* |  _| |  _| (_) | | |_) | (_| | |_| | | | *
//* |_| |_|_|  \___/  | .__/ \__,_|\__|_| |_| *
//*                   |_|                     *
//===- lib/brokerface/fifo_path_win32.cpp ---------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file fifo_path_win32.cpp
/// \brief Portions of the implementation of the broker::fifo_path class that are Windows-specific.

#include "pstore/brokerface/fifo_path.hpp"

#ifdef _WIN32

#    include <sstream>
#    include <thread>

#    include "pstore/support/error.hpp"
#    include "pstore/support/utf.hpp"

namespace pstore {
    namespace brokerface {

        // (dtor)
        // ~~~~~~
        fifo_path::~fifo_path () {}

        // get_default_path
        // ~~~~~~~~~~~~~~~~
        std::string fifo_path::get_default_path () {
            return std::string{R"(\\.\pipe\)"} + default_pipe_name;
        }

        // open_impl
        // ~~~~~~~~~
        auto fifo_path::open_impl () const -> client_pipe {
            auto const path = this->get ();
            auto const path16 = pstore::utf::win32::to16 (path);
            auto fd = client_pipe{::CreateFileW (path16.c_str (), // pipe name
                                                 GENERIC_WRITE,   // write access
                                                 0,               // no sharing
                                                 nullptr,         // default security attributes
                                                 OPEN_EXISTING,   // opens existing pipe
                                                 0,               // default attributes
                                                 nullptr)};       // no template file
            if (!fd.valid ()) {
                // Throw if an error other than ERROR_PIPE_BUSY occurs.
                DWORD const errcode = ::GetLastError ();
                if (errcode != ERROR_PIPE_BUSY && errcode != ERROR_FILE_NOT_FOUND) {
                    std::ostringstream str;
                    str << "Could not open pipe (" << path << ")";
                    raise (::pstore::win32_erc (errcode), str.str ());
                }
            }
            return fd;
        }

        // wait_until_impl
        // ~~~~~~~~~~~~~~~
        void fifo_path::wait_until_impl (std::chrono::milliseconds timeout) const {
            auto const path = this->get ();
            auto const path16 = pstore::utf::win32::to16 (path);

            auto const ms = timeout.count ();
            auto const timeout_ms =
                ms < 1 ? DWORD{NMPWAIT_USE_DEFAULT_WAIT} : static_cast<DWORD> (ms);
            if (!::WaitNamedPipeW (path16.c_str (), timeout_ms)) {
                DWORD const errcode = ::GetLastError ();
                if (errcode != ERROR_SEM_TIMEOUT && errcode != ERROR_FILE_NOT_FOUND) {
                    std::ostringstream str;
                    str << "Could not open pipe (" << path << "): wait time out";
                    raise (::pstore::win32_erc (errcode), str.str ());
                } else {
                    ::Sleep (timeout_ms);
                }
            }
        }
    } // end namespace brokerface
} // end namespace pstore
#endif //_WIN32
