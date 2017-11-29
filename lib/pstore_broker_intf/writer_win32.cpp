//*                _ _             *
//* __      ___ __(_) |_ ___ _ __  *
//* \ \ /\ / / '__| | __/ _ \ '__| *
//*  \ V  V /| |  | | ||  __/ |    *
//*   \_/\_/ |_|  |_|\__\___|_|    *
//*                                *
//===- lib/pstore_broker_intf/writer_win32.cpp ----------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
/// \file writer_win32.cpp
/// \brief Implements the parts of the class which enables a client to send messages to the broker
/// which are Win32-specific.

#include "pstore_broker_intf/writer.hpp"

#ifdef _WIN32

#include "pstore_broker_intf/message_type.hpp"
#include "pstore_support/error.hpp"

namespace pstore {
    namespace broker {

        // write_impl
        // ~~~~~~~~~~
        bool writer::write_impl (message_type const & msg) {
            // Send a message to the pipe server.
            auto bytes_written = DWORD{0};
            BOOL ok = ::WriteFile (fd_.get (),     // pipe handle
                                   &msg,           // message
                                   sizeof (msg),   // message length
                                   &bytes_written, // bytes written
                                   nullptr);       // not overlapped
            if (!ok) {
                DWORD const errcode = ::GetLastError ();
                raise (::pstore::win32_erc (errcode), "WriteFile to pipe failed");
            }
            return true;
        }

    } // namespace broker
} // namespace pstore

#endif // _WIN32
// eof: lib/pstore_broker_intf/writer_win32.cpp
