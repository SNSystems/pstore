//*              _ _    *
//*   __ _ _   _(_) |_  *
//*  / _` | | | | | __| *
//* | (_| | |_| | | |_  *
//*  \__, |\__,_|_|\__| *
//*     |_|             *
//===- lib/http/quit.cpp --------------------------------------------------===//
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
#include "pstore/http/quit.hpp"

#include <sstream>

#ifndef _WIN32
#    include <sys/socket.h>
#    include <sys/types.h>
#endif

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/http/buffered_reader.hpp"
#include "pstore/http/error.hpp"
#include "pstore/http/net_txrx.hpp"
#include "pstore/http/send.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/random.hpp"

namespace pstore {
    namespace httpd {

        void quit (server_status * const http_status) {
            if (!http_status) {
                return;
            }
            if (http_status->shutdown () == server_status::http_state::listening) {
                // Wake up the server by connecting to it.
                broker::socket_descriptor const fd{::socket (AF_INET, SOCK_STREAM, IPPROTO_IP)};
                if (!fd.valid ()) {
                    log (logging::priority::error,
                         "Could not open socket: ", get_last_error ().message ());
                    return;
                }

                struct sockaddr_in sock_addr {};
                // NOLINTNEXTLINE
                sock_addr.sin_port = htons (http_status->port ()); //! OCLINT
                sock_addr.sin_family = AF_INET;
                // NOLINTNEXTLINE
                sock_addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK); //! OCLINT

                log (logging::priority::info, "Connecting");

                if (::connect (fd.native_handle (),
                               reinterpret_cast<struct sockaddr *> (&sock_addr),
                               sizeof (sock_addr)) != 0) {
                    log (logging::priority::error,
                         "Could not connect to localhost: ", get_last_error ().message ());
                    return;
                }

                log (logging::priority::info, "Connected");
            }
        }

    } // end namespace httpd
} // end namespace pstore
