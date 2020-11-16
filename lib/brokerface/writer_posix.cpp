//*                _ _             *
//* __      ___ __(_) |_ ___ _ __  *
//* \ \ /\ / / '__| | __/ _ \ '__| *
//*  \ V  V /| |  | | ||  __/ |    *
//*   \_/\_/ |_|  |_|\__\___|_|    *
//*                                *
//===- lib/brokerface/writer_posix.cpp ------------------------------------===//
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
/// \file writer_posix.cpp
/// \brief Implements the parts of the class which enables a client to send messages to the broker
/// which are POSIX-specific.

#include "pstore/brokerface/writer.hpp"

#ifndef _WIN32

#    include <unistd.h>
#    include "pstore/brokerface/message_type.hpp"
#    include "pstore/support/error.hpp"

namespace pstore {
    namespace brokerface {

        bool writer::write_impl (message_type const & msg) {
            bool const ok = ::write (fd_.native_handle (), &msg, sizeof (msg)) == sizeof (msg);
            if (!ok) {
                int const err = errno;
                if (err != EAGAIN && err != EWOULDBLOCK && err != EPIPE) {
                    raise (errno_erc{err}, "write to broker pipe");
                }
            }
            return ok;
        }

    } // end namespace brokerface
} // end namespace pstore

#endif // _WIN32
