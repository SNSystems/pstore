//===- include/pstore/vacuum/watch.hpp --------------------*- mode: C++ -*-===//
//*                _       _      *
//* __      ____ _| |_ ___| |__   *
//* \ \ /\ / / _` | __/ __| '_ \  *
//*  \ V  V / (_| | || (__| | | | *
//*   \_/\_/ \__,_|\__\___|_| |_| *
//*                               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file watch.hpp

#ifndef PSTORE_VACUUM_WATCH_HPP
#define PSTORE_VACUUM_WATCH_HPP

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

#include "pstore/core/vacuum_intf.hpp"
#include "pstore/os/file.hpp"
#include "pstore/os/shared_memory.hpp"

namespace pstore {
    class database;
}

namespace vacuum {
    struct watch_state {
        bool start_watch = false;
        std::mutex start_watch_mutex;
        std::condition_variable start_watch_cv;
    };
    // TODO: this shouldn't be a global variable.
    extern watch_state wst;

    struct status;
    void watch (std::shared_ptr<pstore::database> from,
                std::unique_lock<pstore::file::range_lock> & lock, status * const st);
} // namespace vacuum

#endif // PSTORE_VACUUM_WATCH_HPP
