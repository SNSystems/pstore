//===- include/pstore/broker/read_loop.hpp ----------------*- mode: C++ -*-===//
//*                     _   _                    *
//*  _ __ ___  __ _  __| | | | ___   ___  _ __   *
//* | '__/ _ \/ _` |/ _` | | |/ _ \ / _ \| '_ \  *
//* | | |  __/ (_| | (_| | | | (_) | (_) | |_) | *
//* |_|  \___|\__,_|\__,_| |_|\___/ \___/| .__/  *
//*                                      |_|     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file read_loop.hpp

#ifndef PSTORE_BROKER_READ_LOOP_HPP
#define PSTORE_BROKER_READ_LOOP_HPP

#include <memory>

#include "pstore/brokerface/fifo_path.hpp"

namespace pstore {
    namespace broker {
        class command_processor;
        class recorder;

        namespace details {

            constexpr unsigned timeout_seconds = 60U;

        } // end namespace details

        void read_loop (brokerface::fifo_path & path, std::shared_ptr<recorder> & record_file,
                        std::shared_ptr<command_processor> cp);

    } // end namespace broker
} // end namespace pstore

#endif // PSTORE_BROKER_READ_LOOP_HPP
