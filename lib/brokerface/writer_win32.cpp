//*                _ _             *
//* __      ___ __(_) |_ ___ _ __  *
//* \ \ /\ / / '__| | __/ _ \ '__| *
//*  \ V  V /| |  | | ||  __/ |    *
//*   \_/\_/ |_|  |_|\__\___|_|    *
//*                                *
//===- lib/brokerface/writer_win32.cpp ------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file writer_win32.cpp
/// \brief Implements the parts of the class which enables a client to send messages to the broker
/// which are Win32-specific.

#include "pstore/brokerface/writer.hpp"

#ifdef _WIN32

#    include "pstore/brokerface/message_type.hpp"
#    include "pstore/support/error.hpp"

namespace pstore {
    namespace brokerface {

        // write_impl
        // ~~~~~~~~~~
        bool writer::write_impl (message_type const & msg) {
            // Send a message to the pipe server.
            auto bytes_written = DWORD{0};
            BOOL ok = ::WriteFile (fd_.native_handle (), // pipe handle
                                   &msg,                 // message
                                   sizeof (msg),         // message length
                                   &bytes_written,       // bytes written
                                   nullptr);             // not overlapped
            if (!ok) {
                DWORD const errcode = ::GetLastError ();
                raise (::pstore::win32_erc (errcode), "WriteFile to pipe failed");
            }
            return true;
        }

    } // end namespace brokerface
} // end namespace pstore

#endif // _WIN32
