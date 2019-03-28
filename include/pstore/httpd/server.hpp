//*                                *
//*  ___  ___ _ ____   _____ _ __  *
//* / __|/ _ \ '__\ \ / / _ \ '__| *
//* \__ \  __/ |   \ V /  __/ |    *
//* |___/\___|_|    \_/ \___|_|    *
//*                                *
//===- include/pstore/httpd/server.hpp ------------------------------------===//
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

#ifndef PSTORE_HTTPD_SERVER_HPP
#define PSTORE_HTTPD_SERVER_HPP

#ifdef _WIN32
using in_port_t = unsigned short;
#else
#    include <netinet/in.h>
#endif

#include "pstore/romfs/romfs.hpp"

namespace pstore {
    namespace httpd {

        int server (in_port_t port_number, romfs::romfs & file_system);
        void quit (in_port_t port_number = 8080);

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTPD_SERVER_HPP
