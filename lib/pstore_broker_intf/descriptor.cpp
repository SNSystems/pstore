//*      _                     _       _              *
//*   __| | ___  ___  ___ _ __(_)_ __ | |_ ___  _ __  *
//*  / _` |/ _ \/ __|/ __| '__| | '_ \| __/ _ \| '__| *
//* | (_| |  __/\__ \ (__| |  | | |_) | || (_) | |    *
//*  \__,_|\___||___/\___|_|  |_| .__/ \__\___/|_|    *
//*                             |_|                   *
//===- lib/pstore_broker_intf/descriptor.cpp ------------------------------===//
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

#include "pstore_broker_intf/descriptor.hpp"

#if defined(_WIN32) && defined(_MSC_VER)

namespace pstore {
    namespace broker {
        namespace details {

            win32_pipe_descriptor_traits::type const win32_pipe_descriptor_traits::invalid =
                INVALID_HANDLE_VALUE;
            win32_pipe_descriptor_traits::type const win32_pipe_descriptor_traits::error =
                INVALID_HANDLE_VALUE;

        } // namespace details
    }     // namespace broker
} // namespace pstore

#endif
// eof: lib/pstore_broker_intf/descriptor.cpp

