//*                _       _      *
//* __      ____ _| |_ ___| |__   *
//* \ \ /\ / / _` | __/ __| '_ \  *
//*  \ V  V / (_| | || (__| | | | *
//*   \_/\_/ \__,_|\__\___|_| |_| *
//*                               *
//===- lib/vacuum/watch.cpp -----------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file watch.cpp
/// \brief Implements the vacuum tool's file watching thread.
///
/// The job of the file-watching thread is to periodically discover whether
/// it is safe for the copy thread to continue its work. It might be unsafe if
/// any of the following happens:
///
/// - A process opens the data store file. We don't want to replace the file with
///   a new, compacted, version under its nose. This would run the risk that it
///   would begin a transaction after the compaction has completed, leading to the
///   loss of that data. The risk of this is lower on Windows because the file
///   system is so unwilling to allow anything to happen to an open file.
/// - A transaction is added to the file. There's a chance that the file is opened
///   and modified and closed in between our probes.
/// - The user renames or delete the file.

#include "pstore/vacuum/watch.hpp"

#include "pstore/core/database.hpp"
#include "pstore/core/file_header.hpp"
#include "pstore/core/time.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/os/thread.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/vacuum/status.hpp"

namespace {
    template <typename Lock>
    bool can_lock (Lock & lock) {
        if (lock.try_lock ()) {
            lock.unlock ();
            return true;
        }
        return false;
    }
} // end anonymous namespace


namespace vacuum {
    watch_state wst;


    void watch (std::shared_ptr<pstore::database> from,
                std::unique_lock<pstore::file::range_lock> & lock, status * const st) {
        pstore::threads::set_name ("watch");
        pstore::create_log_stream ("vacuumd");

        st->watch_running = true;
        using priority = pstore::logger::priority;

        PSTORE_TRY {
            std::unique_lock<decltype (wst.start_watch_mutex)> mlock (wst.start_watch_mutex);
            while (!st->done) {
                // Block until the start_watch condition variable is signaled.
                auto start_time = from->latest_time ();
                while (!wst.start_watch && !st->done) {
                    log (priority::notice, "Waiting until asked to watch by the copy thread...");
                    wst.start_watch_cv.wait_for (mlock, watch_interval,
                                                 [st] () { return wst.start_watch || st->done; });
                }
                wst.start_watch = false;

                auto count = 0U;

                while (!st->done && !st->modified) {
                    log (priority::notice, "watch ... ", count);
                    ++count;

                    auto const current_time = from->latest_time ();
                    bool const file_modified = current_time > start_time;
                    start_time = current_time;

                    if (file_modified || !can_lock (lock)) {
                        log (priority::notice, "Store touched by another process!");
                        st->modified = true;
                    }

                    wst.start_watch_cv.wait_for (mlock, watch_interval);
                }
            }
        }
        // clang-format off
        PSTORE_CATCH (std::exception const & ex, { // clang-format off
            log (priority::error, "An error occurred: ", ex.what ());
        })
        // clang-format off
        PSTORE_CATCH (..., {// clang-format off
            log (priority::error, "Unknown error");
        })
        // clang-format on

        from.reset ();
        log (priority::notice, "Watch thread exiting");
        st->watch_running = false;
    }
} // end namespace vacuum
