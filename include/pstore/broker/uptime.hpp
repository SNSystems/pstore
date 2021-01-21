//*              _   _                 *
//*  _   _ _ __ | |_(_)_ __ ___   ___  *
//* | | | | '_ \| __| | '_ ` _ \ / _ \ *
//* | |_| | |_) | |_| | | | | | |  __/ *
//*  \__,_| .__/ \__|_|_| |_| |_|\___| *
//*       |_|                          *
//===- include/pstore/broker/uptime.hpp -----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#ifndef PSTORE_BROKER_UPTIME_HPP
#define PSTORE_BROKER_UPTIME_HPP

#include <atomic>

#include "pstore/brokerface/pubsub.hpp"
#include "pstore/os/signal_cv.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace broker {

        extern descriptor_condition_variable uptime_cv;
        extern brokerface::channel<descriptor_condition_variable> uptime_channel;

        void uptime (gsl::not_null<std::atomic<bool> *> done);

    } // end namespace broker
} // end namespace pstore

#endif // PSTORE_BROKER_UPTIME_HPP
