//*   __ _                 _                                 *
//*  / _| | ___   ___   __| |  ___  ___ _ ____   _____ _ __  *
//* | |_| |/ _ \ / _ \ / _` | / __|/ _ \ '__\ \ / / _ \ '__| *
//* |  _| | (_) | (_) | (_| | \__ \  __/ |   \ V /  __/ |    *
//* |_| |_|\___/ \___/ \__,_| |___/\___|_|    \_/ \___|_|    *
//*                                                          *
//===- tools/broker_poker/flood_server.cpp --------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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

#include "flood_server.hpp"

#include <string>

#include "pstore_broker_intf/fifo_path.hpp"
#include "pstore_broker_intf/send_message.hpp"
#include "pstore_broker_intf/writer.hpp"
#include "pstore_cmd_util/iota_generator.hpp"
#include "pstore_cmd_util/parallel_for_each.hpp"

void flood_server (pstore::gsl::czstring pipe_path, std::chrono::milliseconds retry_timeout,
                   unsigned max_retries, unsigned long num) {
    std::string path;
    path.reserve (num);

    auto begin = pstore::cmd_util::iota_generator ();
    auto end = pstore::cmd_util::iota_generator (num);
    pstore::cmd_util::parallel_for_each (
        begin, end, 
        [pipe_path, retry_timeout, max_retries](unsigned long count) {
            pstore::broker::fifo_path fifo (pipe_path, retry_timeout, max_retries);
            pstore::broker::writer wr (fifo, retry_timeout, max_retries);

            std::string path;
            path.reserve (count);
            for (auto ctr = 0UL; ctr <= count; ++ctr) {
                path += ctr % 10 + '0';
            }

            constexpr bool error_on_timeout = true;
            pstore::broker::send_message (wr, error_on_timeout, "ECHO", path.c_str ());
        });
}

// eof: tools/broker_poker/flood_server.cpp
