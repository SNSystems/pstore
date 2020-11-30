//*                                                 *
//*  ___  ___ __ ___   _____ _ __   __ _  ___ _ __  *
//* / __|/ __/ _` \ \ / / _ \ '_ \ / _` |/ _ \ '__| *
//* \__ \ (_| (_| |\ V /  __/ | | | (_| |  __/ |    *
//* |___/\___\__,_| \_/ \___|_| |_|\__, |\___|_|    *
//*                                |___/            *
//===- lib/broker/scavenger.cpp -------------------------------------------===//
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
