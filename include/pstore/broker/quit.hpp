//*              _ _    *
//*   __ _ _   _(_) |_  *
//*  / _` | | | | | __| *
//* | (_| | |_| | | |_  *
//*  \__, |\__,_|_|\__| *
//*     |_|             *
//===- include/pstore/broker/quit.hpp -------------------------------------===//
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
/// \file broker/quit.hpp
/// \brief The broker's shutdown thread.

#ifndef PSTORE_BROKER_QUIT_HPP
#define PSTORE_BROKER_QUIT_HPP

#include <atomic>
#include <cstdlib>
#include <memory>
#include <thread>

#include "pstore/support/gsl.hpp"
#include "pstore/support/maybe.hpp"

namespace pstore {
    namespace httpd {

        class server_status;

    } // end namespace httpd

    namespace broker {

        class command_processor;
        class scavenger;

        /// The pretend signal number that's raised when a remote shutdown request is received.
        constexpr int sig_self_quit = -1;

        void shutdown (command_processor * const cp, scavenger * const scav, int const signum,
                       unsigned const num_read_threads,
                       gsl::not_null<maybe<httpd::server_status> *> const http_status,
                       gsl::not_null<std::atomic<bool> *> const uptime_done);

        /// Wakes up the quit thread to start the process of shutting down the server.
        void notify_quit_thread ();

        std::thread create_quit_thread (std::weak_ptr<command_processor> cp,
                                        std::weak_ptr<scavenger> scav, unsigned num_read_threads,
                                        gsl::not_null<maybe<httpd::server_status> *> http_status,
                                        gsl::not_null<std::atomic<bool> *> uptime_done);
    } // end namespace broker
} // end namespace pstore

#endif // PSTORE_BROKER_QUIT_HPP
