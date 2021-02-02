//*      _             _                                            *
//*  ___| |_ __ _ _ __| |_  __   ____ _  ___ _   _ _   _ _ __ ___   *
//* / __| __/ _` | '__| __| \ \ / / _` |/ __| | | | | | | '_ ` _ \  *
//* \__ \ || (_| | |  | |_   \ V / (_| | (__| |_| | |_| | | | | | | *
//* |___/\__\__,_|_|   \__|   \_/ \__,_|\___|\__,_|\__,_|_| |_| |_| *
//*                                                                 *
//===- lib/core/start_vacuum.cpp ------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/core/start_vacuum.hpp"

// Local (public) includes
#include "pstore/brokerface/fifo_path.hpp"
#include "pstore/brokerface/send_message.hpp"
#include "pstore/brokerface/writer.hpp"
#include "pstore/core/database.hpp"

namespace pstore {

    void start_vacuum (database const & db) {
        brokerface::fifo_path const fifo (nullptr);
        brokerface::writer wr (fifo);
        brokerface::send_message (wr, false /*error on timeout*/, "GC", db.path ().c_str ());
    }

} // end namespace pstore
