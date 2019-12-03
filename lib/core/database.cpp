//*      _       _        _                     *
//*   __| | __ _| |_ __ _| |__   __ _ ___  ___  *
//*  / _` |/ _` | __/ _` | '_ \ / _` / __|/ _ \ *
//* | (_| | (_| | || (_| | |_) | (_| \__ \  __/ *
//*  \__,_|\__,_|\__\__,_|_.__/ \__,_|___/\___| *
//*                                             *
//===- lib/core/database.cpp ----------------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
/// \file database.cpp
#include "pstore/core/database.hpp"

#include <cassert>
#include <cstddef>
#include <iterator>
#include <limits>
#include <new>
#include <sstream>
#include <system_error>
#include <type_traits>

#include "pstore/config/config.hpp"
#include "pstore/core/start_vacuum.hpp"
#include "pstore/core/time.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/path.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp"

#include "base32.hpp"
#include "heartbeat.hpp"

namespace {

    pstore::file::range_lock get_vacuum_range_lock (pstore::file::file_base * const file,
                                                    pstore::file::file_base::lock_kind const kind) {
        return {file, sizeof (pstore::header) + offsetof (pstore::lock_block, vacuum_lock),
                sizeof (pstore::lock_block::vacuum_lock), kind};
    }

} // end anonymous namespace

namespace pstore {

    // crc_checks_enabled
    // ~~~~~~~~~~~~~~~~~~
    bool database::crc_checks_enabled () {
#if PSTORE_CRC_CHECKS_ENABLED
        return true;
#else
        return false;
#endif
    }

    // get_shared
    // ~~~~~~~~~~
    shared const * database::get_shared () const {
        assert (shared_.get () != nullptr);
        return shared_.get ();
    }
    shared * database::get_shared () {
        assert (shared_.get () != nullptr);
        return shared_.get ();
    }

    constexpr std::size_t const database::sync_name_length;

    database::database (std::string const & path, access_mode const am,
                        bool const access_tick_enabled)
            : storage_{database::open (path, am)}
            , size_{database::get_footer_pos (*this->file ())} {

        this->finish_init (access_tick_enabled);
    }

    // ~database
    // ~~~~~~~~~
    database::~database () noexcept {
        PSTORE_NO_EX_ESCAPE (this->close ());
    }

    // finish_init
    // ~~~~~~~~~~~
    void database::finish_init (bool access_tick_enabled) {
        (void) access_tick_enabled;

        assert (file ()->is_open ());

        // Build the initial segment address table.
        storage_.update_master_pointers (0);

        trailer::validate (*this, size_.footer_pos ());
        this->protect (address{sizeof (header)}, address{size_.logical_size ()});

        header_ = storage_.address_to_pointer (typed_address<header>::null ());
        sync_name_ = database::build_sync_name (*header_);

#ifdef _WIN32
        shared_ = pstore::shared_memory<pstore::shared> (this->shared_memory_name ());
#endif

        // Put a shared-read lock on the lock_block strcut in the file. We're not going to modify
        // these bytes.
        range_lock_ =
            get_vacuum_range_lock (this->file (), file::file_handle::lock_kind::shared_read);
        lock_ = std::unique_lock<file::range_lock> (range_lock_);

#ifdef _WIN32
        if (access_tick_enabled) {
            heartbeat_ = heartbeat::get ();
            heartbeat_->attach (heartbeat::to_key_type (this), [this](heartbeat::key_type) {
                this->get_shared ()->time = std::time (nullptr);
            });
        }
#endif
    }

    // close
    // ~~~~~
    void database::close () {
        if (!closed_) {
            if (heartbeat_) {
                heartbeat_->detach (heartbeat::to_key_type (this));
            }
            if (modified_ && vacuum_mode_ != vacuum_mode::disabled) {
                start_vacuum (*this);
            }
            closed_ = true;
        }
    }

    // upgrade_to_write_lock
    // ~~~~~~~~~~~~~~~~~~~~~
    std::unique_lock<file::range_lock> * database::upgrade_to_write_lock () {
        // TODO: look at exception-safety in this function.
        lock_.unlock ();
        lock_.release ();
        range_lock_ =
            get_vacuum_range_lock (this->file (), file::file_handle::lock_kind::exclusive_write);
        lock_ = std::unique_lock<file::range_lock> (range_lock_, std::defer_lock);
        return &lock_;
    }

    // clear_index_cache
    // ~~~~~~~~~~~~~~~~~
    void database::clear_index_cache () {
        for (std::shared_ptr<index::index_base> & index : indices_) {
            index.reset ();
        }
    }

    // older_revision_footer_pos
    // ~~~~~~~~~~~~~~~~~~~~~~~~~
    typed_address<trailer> database::older_revision_footer_pos (unsigned const revision) const {
        if (revision == pstore::head_revision || revision > this->get_current_revision ()) {
            raise (pstore::error_code::unknown_revision);
        }

        // Walk backwards down the linked list of revisions to find it.
        typed_address<trailer> footer_pos = size_.footer_pos ();
        for (;;) {
            auto const tail = this->getro (footer_pos);
            unsigned int const tail_revision = tail->a.generation;
            if (revision > tail_revision) {
                raise (pstore::error_code::unknown_revision);
            }
            if (tail_revision == revision) {
                break;
            }

            // TODO: check that footer_pos and generation number are be getting smaller; time must
            // not be getting larger.
            // TODO: prev_generation should probably be atomic
            footer_pos = tail->a.prev_generation;
            trailer::validate (*this, footer_pos);
        }

        return footer_pos;
    }

    // sync
    // ~~~~
    void database::sync (unsigned const revision) {
        // If revision <= current revision then we don't need to start at head! We do so if the
        // revision is later than the current region (that's what is_newer is about with footer_pos
        // tracking the current footer as it moves backwards).
        bool is_newer = false;
        typed_address<trailer> footer_pos = size_.footer_pos ();

        if (revision == head_revision) {
            // If we're asked for the head, then we always need a full check to see if there's
            // a newer revision.
            is_newer = true;
        } else {
            unsigned const current_revision = this->get_current_revision ();
            // An early out if the user requests the same revision that we already have.
            if (revision == current_revision) {
                return;
            }
            if (revision > current_revision) {
                is_newer = true;
            }
        }

        // The transactions form a singly linked list with the newest revision at its head.
        // If we're asked for a revision _newer_ that the currently synced number then we need
        // to hunt backwards starting at the head. Syncing to an older revision is simpler because
        // we can work back from the current revision.
        assert (is_newer || footer_pos != typed_address<trailer>::null ());
        if (is_newer) {
            // This atomic read of footer_pos fixes our view of the head-revision. Any transactions
            // after this point won't be seen by this process.
            auto const new_footer_pos =
                this->getro (typed_address<header>::null ())->footer_pos.load ();

            if (revision == head_revision && new_footer_pos == footer_pos) {
                // We were asked for the head revision but the head turns out to the same
                // as the one to which we're currently synced. The previous early out code didn't
                // have sufficient context to catch this case, but here we do. Nothing more to do.
                return;
            } else {
                footer_pos = new_footer_pos;

                // Always check file size after loading from the footer.
                auto const file_size = storage_.file ()->size ();

                // Perform a proper footer validity check.
                if (footer_pos.absolute () + sizeof (trailer) > file_size) {
                    raise (error_code::footer_corrupt, storage_.file ()->path ());
                }

                // We may need to map additional data from the file. If another process has added
                // data to the store since we opened our connection, then a sync may want to access
                // that new data. Deal with that possibility here.
                storage_.map_bytes (footer_pos.absolute () + sizeof (trailer));

                size_.update_footer_pos (footer_pos);
                trailer::validate (*this, footer_pos);
            }
        }

        // The code above moves to the head revision. If that's what was requested, then we're done.
        // If a specific numbered revision is needed, then we need to search for it.
        if (revision != head_revision) {
            footer_pos = this->older_revision_footer_pos (revision);
        }

        // We must clear the index cache because the current revision has changed.
        this->clear_index_cache ();
        size_.update_footer_pos (footer_pos);
    }

    // build_new_store [static]
    // ~~~~~~~~~~~~~~~
    void database::build_new_store (file::file_base & file) {
        // Write the inital header, lock block, and footer to the file.
        {
            file.seek (0);

            header header{};
            header.footer_pos = typed_address<trailer>::make (leader_size);
            file.write (header);

            lock_block const lb{};
            file.write (lb);

            assert (header.footer_pos.load () == typed_address<trailer>::make (file.tell ()));

            trailer t{};
            std::fill (std::begin (t.a.index_records), std::end (t.a.index_records),
                       typed_address<index::header_block>::null ());
            t.a.time = milliseconds_since_epoch ();
            t.crc = t.get_crc ();
            file.write (t);
        }
        // Make sure that the file is at least large enough for the minimum region size.
        {
            assert (file.size () == leader_size + sizeof (trailer));
            // TODO: ask the region factory what the min region size is.
            if (file.size () < storage::min_region_size && !database::small_files_enabled ()) {
                file.truncate (storage::min_region_size);
            }
        }
    }

    // build_sync_name [static]
    // ~~~~~~~~~~~~~~~
    std::string database::build_sync_name (header const & header) {
        auto name = base32::convert (uint128{header.a.uuid.array ()});
        name.erase (std::min (sync_name_length, name.length ()));
        return name;
    }

    // open [static]
    // ~~~~
    auto database::open (std::string const & path, access_mode const am)
        -> std::shared_ptr<file::file_handle> {

        using present_mode = file::file_handle::present_mode;
        auto const create_mode = file::file_handle::create_mode::open_existing;
        auto const write_mode = (am == access_mode::writable)
                                    ? file::file_handle::writable_mode::read_write
                                    : file::file_handle::writable_mode::read_only;

        auto file = std::make_shared<file::file_handle> (path);
        file->open (create_mode, write_mode, present_mode::allow_not_found);
        if (file->is_open ()) {
            return file;
        }

        if (am != access_mode::writable) {
            raise (std::errc::no_such_file_or_directory, path);
        }

        // We didn't find the file so will have to create a new one.
        //
        // To ensure that this operation is as close to atomic as the host OS allows, we create
        // a temporary file in the same directory but with a random name. (This approach was
        // chosen because this directory most likely has the same permissions and is on the same
        // physical volume as the final file). We then populate that file with the basic data
        // structures and rename it to the final destination.
        {
            auto const temporary_file = std::make_shared<file::file_handle> ();
            temporary_file->open (file::file_handle::unique{}, path::dir_name (path));
            file::deleter temp_file_deleter (temporary_file->path ());
            // Fill the file with its initial contents.
            database::build_new_store (*temporary_file);
            temporary_file->close ();
            temporary_file->rename (path);
            // The temporary file will no longer be found anyway (we just renamed it).
            temp_file_deleter.release ();
        }
        file->open (create_mode, write_mode, file::file_handle::present_mode::must_exist);
        assert (file->is_open () && file->path () == path);
        return file;
    }

    // first_writable_address
    // ~~~~~~~~~~~~~~~~~~~~~~
    address database::first_writable_address () const {
        return (header_->footer_pos.load () + 1).to_address ();
    }

    // get_spanning
    // ~~~~~~~~~~~~
    auto database::get_spanning (address const addr, std::size_t const size, bool const initialized,
                                 bool const writable) const -> std::shared_ptr<void const> {
        // The deleter is called when the shared pointer that we're about to return is
        // released.
        auto deleter = [this, addr, size, writable](std::uint8_t * const p) {
            if (writable) {
                // Check that this code is not trying to write back to read-only storage. This error
                // can occur if a non-const pointer is being destroyed after the containing
                // transaction has been committed.
                assert (addr >= this->first_writable_address ());

                // If we're returning a writable pointer then we must be copy the (potentially
                // modified) contents back to the data store.
                storage_.copy<storage::copy_to_store_traits> (
                    addr, size, p,
                    [](std::uint8_t * const dest, std::uint8_t const * const src,
                       std::size_t const n) { std::memcpy (dest, src, n); });
            }
            delete[](p);
        };

        // Create the memory block that we'll be returning, attaching a suitable deleter
        // which will be responsible for copying the data back to the store (if we're providing
        // a writable pointer).
        std::shared_ptr<std::uint8_t> const result{new std::uint8_t[size], deleter};

        if (initialized) {
            // Copy from the data store's regions to the newly allocated memory block.
            storage_.copy<storage::copy_from_store_traits> (
                addr, size, result.get (),
                [](std::uint8_t const * const src, std::uint8_t * const dest, std::size_t const n) {
                    std::memcpy (dest, src, n);
                });
        }
        return std::static_pointer_cast<void const> (result);
    }

    // get
    // ~~~
    auto database::get (address const addr, std::size_t const size, bool const initialized,
                        bool const writable) const -> std::shared_ptr<void const> {
        if (closed_) {
            raise (pstore::error_code::store_closed);
        }

        if (writable && addr < first_writable_address ()) {
            raise (error_code::read_only_address,
                   "An attempt was made to write to read-only storage");
        }
        std::uint64_t const start = addr.absolute ();
        std::uint64_t const logical_size = size_.logical_size ();
        if (start > logical_size || size > logical_size - start) {
            raise (error_code::bad_address);
        }

        if (storage_.request_spans_regions (addr, size)) {
            return this->get_spanning (addr, size, initialized, writable);
        }
        return storage_.address_to_pointer (addr);
    }

    // allocate
    // ~~~~~~~~
    pstore::address database::allocate (std::uint64_t const bytes, unsigned const align) {
        assert (is_power_of_two (align));
        if (closed_) {
            raise (error_code::store_closed);
        }
        modified_ = true;

        std::uint64_t const result = size_.logical_size ();
        assert (result >= size_.footer_pos ().absolute () + sizeof (trailer));

        std::uint64_t const extra_for_alignment = calc_alignment (result, std::uint64_t{align});
        assert (extra_for_alignment < align);

        // Increase the number of bytes being requested by enough to ensure that result is
        // properly aligned.
        std::uint64_t const new_logical_size = result + bytes + extra_for_alignment;

        // Memory map additional space if necessary.
        storage_.map_bytes (new_logical_size);

        size_.update_logical_size (new_logical_size);
        if (database::small_files_enabled ()) {
            this->file ()->truncate (new_logical_size);
        }

        // Bump 'result' up by the number of alignment bytes that we're adding to ensure
        // that it's properly aligned.
        return address{result + extra_for_alignment};
    }

    // truncate
    // ~~~~~~~~
    void database::truncate (std::uint64_t const size) {
        if (closed_) {
            raise (error_code::store_closed);
        }
        modified_ = true;

        // Memory map space if necessary.
        storage_.map_bytes (size);

        size_.truncate_logical_size (size);
        if (database::small_files_enabled ()) {
            this->file ()->truncate (size);
        }
    }

    // set_new_footer
    // ~~~~~~~~~~~~~~
    void database::set_new_footer (typed_address<trailer> const new_footer_pos) {
        size_.update_footer_pos (new_footer_pos);

        // Finally (this should be the last thing we do), point the file header at the new
        // footer. Any other threads/processes will now see our new transaction as the state
        // of the database.

        header_->footer_pos = new_footer_pos;
    }

} // end namespace pstore
