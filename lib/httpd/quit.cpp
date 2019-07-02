//*              _ _    *
//*   __ _ _   _(_) |_  *
//*  / _` | | | | | __| *
//* | (_| | |_| | | |_  *
//*  \__, |\__,_|_|\__| *
//*     |_|             *
//===- lib/httpd/quit.cpp -------------------------------------------------===//
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
#include "pstore/httpd/quit.hpp"

#include <sstream>

#ifndef _WIN32
#    include <sys/socket.h>
#    include <sys/types.h>
#endif

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/httpd/buffered_reader.hpp"
#include "pstore/httpd/net_txrx.hpp"
#include "pstore/httpd/send.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/random.hpp"

namespace {

    // get_last_error
    // ~~~~~~~~~~~~~~
    inline std::error_code get_last_error () noexcept {
#ifdef _WIN32
        return make_error_code (pstore::win32_erc{static_cast<DWORD> (WSAGetLastError ())});
#else
        return make_error_code (std::errc (errno));
#endif // !_WIN32
    }

} // end anonymous namespace

namespace pstore {
    namespace httpd {

        void quit (gsl::not_null<server_status *> http_status) {
            using broker::socket_descriptor;

            auto old_state = http_status->state.load ();
            http_status->shutdown ();
            if (old_state == server_status::http_state::listening) {
                socket_descriptor fd{::socket (AF_INET, SOCK_STREAM, IPPROTO_IP)};
                if (!fd.valid ()) {
                    log (logging::priority::error, "Could not open socket ",
                         get_last_error ().message ());
                    return;
                }

                struct sockaddr_in sock_addr {};
                sock_addr.sin_port = htons (http_status->port); // NOLINT
                sock_addr.sin_family = AF_INET;
                sock_addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK); // NOLINT

                log (logging::priority::info, "Connecting");

                // FIXME: timeout!
                if (::connect (fd.native_handle (),
                               reinterpret_cast<struct sockaddr *> (&sock_addr),
                               sizeof (sock_addr)) != 0) {
                    log (logging::priority::error, "Could not connect to localhost");
                    return;
                }

                log (logging::priority::info, "Connected");
            }
        }

    } // end namespace httpd
} // end namespace pstore
