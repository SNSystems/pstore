//*              _   _                 *
//*  _   _ _ __ | |_(_)_ __ ___   ___  *
//* | | | | '_ \| __| | '_ ` _ \ / _ \ *
//* | |_| | |_) | |_| | | | | | |  __/ *
//*  \__,_| .__/ \__|_|_| |_| |_|\___| *
//*       |_|                          *
//===- lib/httpd/uptime.cpp -----------------------------------------------===//
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
#include "pstore/httpd/uptime.hpp"

#include <sstream>
#include <thread>

#include "pstore/json/json.hpp"
#include "pstore/json/dom_types.hpp"
#include "pstore/support/thread.hpp"

namespace {

#ifndef NDEBUG
    bool is_valid_json (std::string const & str) {
        pstore::json::parser<pstore::json::null_output> p;
        p.input (pstore::gsl::make_span (str));
        p.eof ();
        return !p.has_error ();
    }
#endif

} // end anonymous namespace


namespace pstore {
    namespace httpd {

        descriptor_condition_variable uptime_cv;
        channel<descriptor_condition_variable> uptime_channel (&uptime_cv);

        void uptime (gsl::not_null<bool *> done) {
            threads::set_name ("uptime");

            auto count = 0U;
            auto until = std::chrono::system_clock::now ();
            while (!*done) {
                until += std::chrono::seconds{1};
                std::this_thread::sleep_until (until);
                ++count;

                uptime_channel.publish ([count]() {
                    std::ostringstream os;
                    os << "{ \"uptime\": " << count << " }";
                    std::string const & str = os.str ();
                    assert (is_valid_json (str));
                    return str;
                });
            }
        }

    } // end namespace httpd
} // end namespace pstore
