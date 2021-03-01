//===- include/pstore/broker/quit.hpp ---------------------*- mode: C++ -*-===//
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
    namespace http {

        class server_status;

    } // end namespace http

    namespace broker {

        class command_processor;
        class scavenger;

        /// The pretend signal number that's raised when a remote shutdown request is received.
        constexpr int sig_self_quit = -1;

        void shutdown (command_processor * const cp, scavenger * const scav, int const signum,
                       unsigned const num_read_threads,
                       gsl::not_null<maybe<http::server_status> *> const http_status,
                       gsl::not_null<std::atomic<bool> *> const uptime_done);

        /// Wakes up the quit thread to start the process of shutting down the server.
        void notify_quit_thread ();

        std::thread create_quit_thread (std::weak_ptr<command_processor> cp,
                                        std::weak_ptr<scavenger> scav, unsigned num_read_threads,
                                        gsl::not_null<maybe<http::server_status> *> http_status,
                                        gsl::not_null<std::atomic<bool> *> uptime_done);
    } // end namespace broker
} // end namespace pstore

#endif // PSTORE_BROKER_QUIT_HPP
