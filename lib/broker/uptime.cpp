//*              _   _                 *
//*  _   _ _ __ | |_(_)_ __ ___   ___  *
//* | | | | '_ \| __| | '_ ` _ \ / _ \ *
//* | |_| | |_) | |_| | | | | | |  __/ *
//*  \__,_| .__/ \__|_|_| |_| |_|\___| *
//*       |_|                          *
//===- lib/broker/uptime.cpp ----------------------------------------------===//
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
#include "pstore/broker/uptime.hpp"

#include <sstream>
#include <thread>

#include "pstore/json/utility.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/os/thread.hpp"

namespace pstore {
    namespace broker {

        descriptor_condition_variable uptime_cv;
        channel<descriptor_condition_variable> uptime_channel (&uptime_cv);

        void uptime (gsl::not_null<std::atomic<bool> *> done) {
            log (logging::priority::info, "uptime 1 second tick starting");

            auto seconds = std::uint64_t{0};
            auto until = std::chrono::system_clock::now ();
            while (!*done) {
                until += std::chrono::seconds{1};
                std::this_thread::sleep_until (until);
                ++seconds;

                uptime_channel.publish ([seconds]() {
                    std::ostringstream os;
                    os << "{ \"uptime\": " << seconds << " }";
                    std::string const & str = os.str ();
                    assert (json::is_valid (str));
                    return str;
                });
            }

            log (logging::priority::info, "uptime thread exiting");
        }

    } // namespace broker
} // end namespace pstore
