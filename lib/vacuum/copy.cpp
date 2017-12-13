//*                         *
//*   ___ ___  _ __  _   _  *
//*  / __/ _ \| '_ \| | | | *
//* | (_| (_) | |_) | |_| | *
//*  \___\___/| .__/ \__, | *
//*           |_|    |___/  *
//===- lib/vacuum/copy.cpp ------------------------------------------------===//
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
/// \file copy.cpp

#include "vacuum/copy.hpp"

#include <cstdio>
#include <memory>
#include <sstream>
#include <thread>

#include "pstore/database.hpp"
#include "pstore/hamt_map.hpp"
#include "pstore/index_types.hpp"
#include "pstore/make_unique.hpp"
#include "pstore/transaction.hpp"
#include "pstore_support/logging.hpp"
#include "pstore_support/portab.hpp"
#include "pstore_support/thread.hpp"

#include "vacuum/status.hpp"
#include "vacuum/user_options.hpp"
#include "vacuum/watch.hpp"

#ifdef PSTORE_CPP_EXCEPTIONS
#define TRY try
#define CATCH(ex, code) catch (ex) code
#else
#define TRY
#define CATCH(ex, code)
#endif

namespace {
    void stop (vacuum::status * const st) {
        st->done = true;

        // Wake up the watch thread immediately.
        std::lock_guard<std::mutex> lk (vacuum::wst.start_watch_mutex);
        vacuum::wst.start_watch_cv.notify_all ();
    }

    void precopy_delay (vacuum::status * const st) {
        using vacuum::wst;

        pstore::logging::log (pstore::logging::priority::notice,
                              "waiting before starting to vacuum...");

        std::unique_lock<std::mutex> mlock (wst.start_watch_mutex);

        auto const wait_until = std::chrono::system_clock::now () + vacuum::initial_delay;
        for (auto now = std::chrono::system_clock::now (); now < wait_until && !st->done;
             now = std::chrono::system_clock::now ()) {
            // FIXME: watch the file for modification. If it is touched, reset wait_until.
            auto remaining = wait_until - now;
            pstore::logging::log (
                pstore::logging::priority::notice, "Pre-copy delay. Remaining (ms): ",
                std::chrono::duration_cast<std::chrono::milliseconds> (remaining).count ());
            wst.start_watch_cv.wait_for (mlock, vacuum::watch_interval);
        }
    }
} // end anonymous namespace

namespace vacuum {
    void copy (std::shared_ptr<pstore::database> source, status * const st,
               user_options const & opt) {
        pstore::threads::set_name ("copy");
        pstore::logging::create_log_stream ("vacuumd");

        TRY {
            pstore::logging::log (pstore::logging::priority::notice, "Copy thread started");
            while (!st->done) {
                pstore::logging::log (pstore::logging::priority::notice,
                                      "Waiting before beginning to vacuum...");

                if (opt.daemon_mode) {
                    precopy_delay (st);
                }
                if (st->done) {
                    return;
                }

                pstore::logging::log (pstore::logging::priority::notice, "Collecting...");

                // Tell the "watch" thread to start monitoring the store for changes.
                {
                    std::lock_guard<std::mutex> lk (wst.start_watch_mutex);
                    source->sync ();
                    wst.start_watch = true;
                    st->modified = false;
                    wst.start_watch_cv.notify_all ();
                }

                bool copy_aborted = false;
                // TODO: a new constructor to make a uniquely named file in the same directory as
                // 'from'
                auto destination = std::make_unique<pstore::database> (
                    source->path () + ".gc", pstore::database::access_mode::writable);

                // We don't want our pristine new store to be vacuumed; it doesn't need it.
                destination->set_vacuum_mode (pstore::database::vacuum_mode::disabled);

                pstore::index::write_index * const source_names =
                    pstore::index::get_write_index (*source);
                if (source_names == nullptr) {
                    pstore::logging::log (pstore::logging::priority::error,
                                          "Names index was not found in source store");
                    stop (st);
                    return;
                }

                pstore::index::write_index * const destination_names =
                    pstore::index::get_write_index (*destination);

                if (!st->done) {
                    auto transaction = pstore::begin (*destination);

                    for (auto const & kvp : *source_names) {
                        std::string const & key = kvp.first;
                        pstore::extent const & extent = kvp.second;

                        // FIXME: need to know the data's original alignment!
                        pstore::address const addr =
                            transaction.allocate (extent.size, 1 /*align*/);
                        // Copy from the source file to the data store.
                        std::memcpy (transaction.getrw (addr, extent.size).get (),
                                     source->getro (extent).get (), extent.size);

                        destination_names->insert_or_assign (transaction, key,
                                                             pstore::extent{addr, extent.size});

                        // Has the watch thread asked us to abort the copy?
                        if (st->modified) {
                            copy_aborted = true;
                            pstore::logging::log (pstore::logging::priority::notice,
                                                  "Store was modified during vacuuming: aborted.");
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
                    pstore::logging::log (pstore::logging::priority::notice, "Vacuuming complete");
                    stop (st);
                    while (st->watch_running) {
                        std::this_thread::sleep_for (std::chrono::microseconds (10));
                    }

                    // FIXME: wait for the watch thread to close its connection to the source store.

                    // FIXME: take the source global store mutex.
                    // FIXME: take the exclusive write lock.
                    std::string const destination_path = destination->path ();
                    std::string const source_path = source->path ();
                    destination.reset (); // Close the target data store
                    // assert that there's a single reference to the source pointer.
                    source.reset ();

                    // TODO: need to have some sort of lock on the stores here.
                    // FIXME: use file.h/rename()!
                    pstore::file::rename (destination_path, source_path);
                }
            }
        }
        // clang-format off
        CATCH (std::exception const & ex, {
            pstore::logging::log (pstore::logging::priority::error, "An error occurred: ",
                                  ex.what ());
        })
        CATCH (..., {
            pstore::logging::log (pstore::logging::priority::error, "Unknown error"); })
            pstore::logging::log (pstore::logging::priority::notice, "Copy thread exiting");
        }
        // clang-format on
} // end namespace vacuum
// eof: lib/vacuum/copy.cpp
