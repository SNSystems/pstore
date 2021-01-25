//*                                                 *
//*  ___  ___ __ ___   _____ _ __   __ _  ___ _ __  *
//* / __|/ __/ _` \ \ / / _ \ '_ \ / _` |/ _ \ '__| *
//* \__ \ (_| (_| |\ V /  __/ | | | (_| |  __/ |    *
//* |___/\___\__,_| \_/ \___|_| |_|\__, |\___|_|    *
//*                                |___/            *
//===- lib/broker/scavenger.cpp -------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file scavenger.cpp

#include "pstore/broker/scavenger.hpp"

#include <chrono>

#include "pstore/broker/command.hpp"
#include "pstore/broker/globals.hpp"
#include "pstore/os/logging.hpp"

namespace pstore {
    namespace broker {

        // thread_entry
        // ~~~~~~~~~~~~
        void scavenger::thread_entry () {
            try {
                std::unique_lock<decltype (mut_)> lock (mut_);
                auto const sleep_time =
                    std::chrono::seconds (10 * 60); // TODO: make this configurable by the user.
                for (;;) {
                    cv_.wait_for (lock, sleep_time);
                    log (logger::priority::info, "begin scavenging");
                    if (done) {
                        break;
                    }

                    // If the command processor still exists, ask it to scavenge any stale records.
                    if (auto const scp = cp_.lock ()) {
                        scp->scavenge ();
                    }

                    log (logger::priority::info, "scavenging done");
                }
            } catch (std::exception const & ex) {
                log (logger::priority::error, "error:", ex.what ());
            } catch (...) {
                log (logger::priority::error, "unknown exception");
            }
            log (logger::priority::info, "scavenger thread exiting");
        }

        // shutdown
        // ~~~~~~~~
        void scavenger::shutdown () {
            std::lock_guard<decltype (mut_)> const lock{mut_};
            cv_.notify_all ();
        }

    } // namespace broker
} // namespace pstore
