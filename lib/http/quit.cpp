//===- lib/http/quit.cpp --------------------------------------------------===//
//*              _ _    *
//*   __ _ _   _(_) |_  *
//*  / _` | | | | | __| *
//* | (_| | |_| | | |_  *
//*  \__, |\__,_|_|\__| *
//*     |_|             *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/http/quit.hpp"

#include <sstream>

#ifndef _WIN32
#    include <sys/socket.h>
#    include <sys/types.h>
#endif

#include "pstore/http/buffered_reader.hpp"
#include "pstore/http/error.hpp"
#include "pstore/http/net_txrx.hpp"
#include "pstore/http/send.hpp"
#include "pstore/os/descriptor.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/random.hpp"

namespace pstore {
    namespace http {

        void quit (gsl::not_null<maybe<server_status> *> http_status) {
            if (!http_status->has_value ()) {
                return;
            }
            auto & s = **http_status;
            if (s.set_state_to_shutdown () == server_status::http_state::listening) {
                // The server was previously in the listening state so we must wake it up by
                // connecting.
                socket_descriptor const fd{::socket (AF_INET, SOCK_STREAM, IPPROTO_IP)};
                if (!fd.valid ()) {
                    log (logger::priority::error,
                         "Could not open socket: ", get_last_error ().message ());
                    return;
                }

                struct sockaddr_in sock_addr {};
                // NOLINTNEXTLINE
                sock_addr.sin_port = htons (s.port ()); //! OCLINT
                sock_addr.sin_family = AF_INET;
                // NOLINTNEXTLINE
                sock_addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK); //! OCLINT

                log (logger::priority::info, "Connecting");

                if (::connect (fd.native_handle (),
                               reinterpret_cast<struct sockaddr *> (&sock_addr),
                               sizeof (sock_addr)) != 0) {
                    // Perhaps another connection happened before the one that we're attempting here
                    // and the server has already shutdown.
                    log (logger::priority::error,
                         "Could not connect to localhost: ", get_last_error ().message ());
                    return;
                }

                log (logger::priority::info, "Connected");
            }
        }

    } // end namespace http
} // end namespace pstore
