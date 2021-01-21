//*                                                               _  *
//*  _ __ ___   ___  ___ ___  __ _  __ _  ___   _ __   ___   ___ | | *
//* | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ | '_ \ / _ \ / _ \| | *
//* | | | | | |  __/\__ \__ \ (_| | (_| |  __/ | |_) | (_) | (_) | | *
//* |_| |_| |_|\___||___/___/\__,_|\__, |\___| | .__/ \___/ \___/|_| *
//*                                |___/       |_|                   *
//===- lib/broker/message_pool.cpp ----------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file message_pool.cpp

#include "pstore/broker/message_pool.hpp"

namespace pstore {
    namespace broker {

        message_pool pool;

    } // namespace broker
} // namespace pstore
