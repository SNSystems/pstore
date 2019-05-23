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
#include "pstore/support/logging.hpp"
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

    /// Produces a 16 digit random hex number. This is sent along with a GET of /cmd/quit to
    /// dissuade the status server from trivially shutdown by a client.
    std::string make_quit_magic () {
        std::string resl;
        pstore::random_generator<unsigned> rnd;

        auto num2hex = [](unsigned a) {
            assert (a < 16);
            return static_cast<char> (a < 10 ? a + '0' : a - 10 + 'a');
        };
        resl.reserve (16);
        for (int j = 0; j < 16; ++j) {
            resl += num2hex (rnd.get (16U));
        }
        return resl;
    }

} // end anonymous namespace

namespace pstore {
    namespace httpd {

        std::string get_quit_magic () {
            static std::string const quit_magic = make_quit_magic ();
            return quit_magic;
        }

        void quit (in_port_t port_number) {
            using broker::socket_descriptor;

            socket_descriptor fd{::socket (AF_INET, SOCK_STREAM, IPPROTO_IP)};
            if (!fd.valid ()) {
                log (logging::priority::error, "Could not open socket ",
                     get_last_error ().message ());
                return;
            }

            struct sockaddr_in sock_addr {};
            sock_addr.sin_port = htons (port_number); // NOLINT
            sock_addr.sin_family = AF_INET;
            sock_addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK); // NOLINT

            log (logging::priority::info, "Connecting");

            if (::connect (fd.native_handle (), reinterpret_cast<struct sockaddr *> (&sock_addr),
                           sizeof (sock_addr)) != 0) {
                log (logging::priority::error, "Could not connect to localhost");
                return;
            }

            log (logging::priority::info, "Connected");

            std::ostringstream str;
            str << "GET /cmd/quit?magic=" << get_quit_magic () << " HTTP1.1 " << crlf
                << "Host: localhost:" << port_number << crlf << "Connection: close" << crlf << crlf;
            std::string const & req = str.str ();
            send (net::network_sender, std::ref (fd), req);

            std::string response;
            std::array<char, 256> buffer;
            auto reader = make_buffered_reader<socket_descriptor &> (net::refiller);
            for (;;) {
                error_or_n<socket_descriptor &, gsl::span<char>> const eo =
                    reader.get_span (fd, gsl::make_span (buffer));
                if (!eo) {
                    log (logging::priority::error, "Read error: ", eo.get_error ().message ());
                    return;
                }

                fd = std::move (std::get<0> (eo));
                auto const & span = std::get<1> (eo);
                if (span.size () == 0) {
                    break;
                }
                response.append (span.begin (), span.end ());
            }
            log (logging::priority::info, "Response: ", response);
        }

    } // end namespace httpd
} // end namespace pstore
