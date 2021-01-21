//*              _   _                 *
//*  _   _ _ __ | |_(_)_ __ ___   ___  *
//* | | | | '_ \| __| | '_ ` _ \ / _ \ *
//* | |_| | |_) | |_| | | | | | |  __/ *
//*  \__,_| .__/ \__|_|_| |_| |_|\___| *
//*       |_|                          *
//===- lib/broker/uptime.cpp ----------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/broker/uptime.hpp"

#include <sstream>
#include <thread>

#include "pstore/json/utility.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/os/thread.hpp"

namespace pstore {
    namespace broker {

        descriptor_condition_variable uptime_cv;
        brokerface::channel<descriptor_condition_variable> uptime_channel (&uptime_cv);

        void uptime (gsl::not_null<std::atomic<bool> *> const done) {
            log (logger::priority::info, "uptime 1 second tick starting");

            auto seconds = std::uint64_t{0};
            auto until = std::chrono::system_clock::now ();
            while (!*done) {
                until += std::chrono::seconds{1};
                std::this_thread::sleep_until (until);
                ++seconds;

                uptime_channel.publish ([seconds] () {
                    std::ostringstream os;
                    os << "{ \"uptime\": " << seconds << " }";
                    std::string const & str = os.str ();
                    PSTORE_ASSERT (json::is_valid (str));
                    return str;
                });
            }

            log (logger::priority::info, "uptime thread exiting");
        }

    } // namespace broker
} // end namespace pstore
