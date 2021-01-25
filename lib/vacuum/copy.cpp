//*                         *
//*   ___ ___  _ __  _   _  *
//*  / __/ _ \| '_ \| | | | *
//* | (_| (_) | |_) | |_| | *
//*  \___\___/| .__/ \__, | *
//*           |_|    |___/  *
//===- lib/vacuum/copy.cpp ------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file copy.cpp

#include "pstore/vacuum/copy.hpp"

#include <cstdio>
#include <memory>
#include <sstream>
#include <thread>

#include "pstore/core/hamt_map.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/os/logging.hpp"
#include "pstore/os/thread.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/vacuum/status.hpp"
#include "pstore/vacuum/user_options.hpp"
#include "pstore/vacuum/watch.hpp"

namespace {

    void stop (vacuum::status * const st) {
        st->done = true;

        // Wake up the watch thread immediately.
        auto & wst = vacuum::wst;
        std::lock_guard<decltype (wst.start_watch_mutex)> const lock{wst.start_watch_mutex};
        wst.start_watch_cv.notify_one ();
    }


    // Tell the "watch" thread to start monitoring the store for changes.
    void start_watching (std::shared_ptr<pstore::database> const & source,
                         vacuum::status * const st) {
        auto & wst = vacuum::wst;
        std::lock_guard<decltype (wst.start_watch_mutex)> const lock{wst.start_watch_mutex};
        source->sync ();
        wst.start_watch = true;
        st->modified = false;
        wst.start_watch_cv.notify_one ();
    }


    void precopy_delay (vacuum::status * const st) {
        auto & wst = vacuum::wst;
        using priority = pstore::logger::priority;
        log (priority::notice, "waiting before starting to vacuum...");

        std::unique_lock<decltype (wst.start_watch_mutex)> lock (wst.start_watch_mutex);

        auto const wait_until = std::chrono::system_clock::now () + vacuum::initial_delay;
        for (auto now = std::chrono::system_clock::now (); now < wait_until && !st->done;
             now = std::chrono::system_clock::now ()) {
            // TODO: watch the file for modification. If it is touched, reset wait_until.
            auto const remaining = wait_until - now;
            log (priority::notice, "Pre-copy delay. Remaining (ms): ",
                 std::chrono::duration_cast<std::chrono::milliseconds> (remaining).count ());
            wst.start_watch_cv.wait_for (lock, vacuum::watch_interval);
        }
    }

} // end anonymous namespace

namespace vacuum {

    void copy (std::shared_ptr<pstore::database> source, status * const st,
               user_options const & opt) {
        pstore::threads::set_name ("copy");
        pstore::create_log_stream ("vacuumd");
        using priority = pstore::logger::priority;

        PSTORE_TRY {
            log (priority::notice, "Copy thread started");
            while (!st->done) {
                log (priority::notice, "Waiting before beginning to vacuum...");

                if (opt.daemon_mode) {
                    precopy_delay (st);
                }
                if (st->done) {
                    return;
                }

                log (priority::notice, "Collecting...");

                // Tell the "watch" thread to start monitoring the store for changes.
                start_watching (source, st);

                bool copy_aborted = false;
                // TODO: a new constructor to make a uniquely named file in the same directory as
                // 'from'
                auto destination = std::make_unique<pstore::database> (
                    source->path () + ".gc", pstore::database::access_mode::writable);

                // We don't want our pristine new store to be vacuumed; it doesn't need it.
                destination->set_vacuum_mode (pstore::database::vacuum_mode::disabled);

                std::shared_ptr<pstore::index::write_index const> const source_names =
                    pstore::index::get_index<pstore::trailer::indices::write> (*source);
                if (source_names == nullptr) {
                    log (priority::error, "Names index was not found in source store");
                    stop (st);
                    return;
                }

                std::shared_ptr<pstore::index::write_index> const destination_names =
                    pstore::index::get_index<pstore::trailer::indices::write> (*destination);

                if (!st->done) {
                    auto transaction = pstore::begin (*destination);

                    for (auto const & kvp : source_names->make_range (*source)) {
                        std::string const & key = kvp.first;
                        pstore::extent<char> const & extent = kvp.second;

                        pstore::address const addr =
                            transaction.allocate (extent.size, 1 /*align*/);
                        // Copy from the source file to the data store.
                        std::memcpy (transaction.getrw (addr, extent.size).get (),
                                     source->getro (extent).get (), extent.size);

                        destination_names->insert_or_assign (
                            transaction, key,
                            make_extent (pstore::typed_address<char> (addr), extent.size));

                        // Has the watch thread asked us to abort the copy?
                        if (st->modified) {
                            copy_aborted = true;
                            log (priority::notice, "Store was modified during vacuuming: aborted.");
                            break;
                        }
                    }

                    if (copy_aborted) {
                        transaction.rollback ();
                    } else {
                        transaction.commit ();
                    }

                    destination->close ();
                }

                if (!copy_aborted) {
                    log (priority::notice, "Vacuuming complete");
                    stop (st);
                    while (st->watch_running) {
                        std::this_thread::sleep_for (std::chrono::microseconds (10));
                    }

                    // TODO: wait for the watch thread to close its connection to the source store.

                    pstore::file::file_handle destination_file{destination->path ()};
                    std::string const source_path = source->path ();
                    destination.reset (); // Close the target data store
                    // assert that there's a single reference to the source pointer.
                    source.reset ();

                    destination_file.rename (source_path);
                }
            }
        }
        // clang-format off
        PSTORE_CATCH (std::exception const & ex, { // clang-format on
            log (priority::error, "An error occurred: ", ex.what ());
        })
        // clang-format off
        PSTORE_CATCH (..., { // clang-format on
            log (priority::error, "Unknown error");
        })

        log (priority::notice, "Copy thread exiting");
    }

} // end namespace vacuum
