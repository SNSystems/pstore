//===- tools/broker_poker/flood_server.cpp --------------------------------===//
//*   __ _                 _                                 *
//*  / _| | ___   ___   __| |  ___  ___ _ ____   _____ _ __  *
//* | |_| |/ _ \ / _ \ / _` | / __|/ _ \ '__\ \ / / _ \ '__| *
//* |  _| | (_) | (_) | (_| | \__ \  __/ |   \ V /  __/ |    *
//* |_| |_|\___/ \___/ \__,_| |___/\___|_|    \_/ \___|_|    *
//*                                                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "flood_server.hpp"

#include <string>

#include "pstore/brokerface/fifo_path.hpp"
#include "pstore/brokerface/send_message.hpp"
#include "pstore/brokerface/writer.hpp"
#include "pstore/support/parallel_for_each.hpp"

#include "iota_generator.hpp"

void flood_server (pstore::gsl::czstring pipe_path, std::chrono::milliseconds retry_timeout,
                   unsigned long num) {
    pstore::parallel_for_each (
        iota_generator (), iota_generator (num), [pipe_path, retry_timeout] (unsigned long count) {
            pstore::brokerface::fifo_path fifo (pipe_path, retry_timeout,
                                                pstore::brokerface::fifo_path::infinite_retries);
            pstore::brokerface::writer wr (fifo, retry_timeout,
                                           pstore::brokerface::writer::infinite_retries);

            std::string path;
            path.reserve (count);
            for (auto ctr = 0UL; ctr <= count; ++ctr) {
                path += ctr % 10 + '0';
            }

            constexpr bool error_on_timeout = true;
            pstore::brokerface::send_message (wr, error_on_timeout, "ECHO", path.c_str ());
        });
}
