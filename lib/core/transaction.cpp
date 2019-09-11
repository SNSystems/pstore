//*  _                                  _   _              *
//* | |_ _ __ __ _ _ __  ___  __ _  ___| |_(_) ___  _ __   *
//* | __| '__/ _` | '_ \/ __|/ _` |/ __| __| |/ _ \| '_ \  *
//* | |_| | | (_| | | | \__ \ (_| | (__| |_| | (_) | | | | *
//*  \__|_|  \__,_|_| |_|___/\__,_|\___|\__|_|\___/|_| |_| *
//*                                                        *
//===- lib/core/transaction.cpp -------------------------------------------===//
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
/// \file transaction.cpp
/// \brief Data store transaction implementation
#include "pstore/core/transaction.hpp"

#include <utility>

#include "pstore/core/index_types.hpp"

namespace pstore {
    transaction_base::transaction_base (transaction_base && rhs) noexcept
            : db_{rhs.db_}
            , size_{rhs.size_}
            , dbsize_{rhs.dbsize_}
            , first_ (rhs.first_) {
        rhs.first_ = address::null ();
        assert (!rhs.is_open ());
    }

    // allocate
    // ~~~~~~~~
    address transaction_base::allocate (std::uint64_t size, unsigned align) {
        database & db = this->db ();
        auto const old_size = db.size ();
        address const result = db.allocate (size, align);
        if (first_ == address::null ()) {
            if (size_ != 0) {
                // Cannot allocate data after a transaction has been committed
                raise (error_code::cannot_allocate_after_commit);
            }
            first_ = result;
        }

        // Increase the transaction size by the actual number of bytes
        // allocated. This may be greater than the number requested to
        // allow for alignment.
        auto const bytes_allocated = db.size () - old_size;
        assert (bytes_allocated >= size);
        size_ += bytes_allocated;
        return result;
    }

    // alloc_rw
    // ~~~~~~~~
    std::pair<std::shared_ptr<void>, address> transaction_base::alloc_rw (std::size_t size,
                                                                          unsigned align) {
        address const addr = this->allocate (size, align);
        // We call database::get() with the initialized parameter set to false because this
        // is new storage: there's no need to copy its existing contents if the block spans
        // more than one region.
        auto ptr = std::const_pointer_cast<void> (db_.get (addr, size,
                                                           false,  // initialized?
                                                           true)); // writable?
        return {ptr, addr};
    }

    // commit
    // ~~~~~~
    transaction_base & transaction_base::commit () {
        if (this->is_open ()) {
            database & db = this->db ();

            // We're going to write to the header, but this must be the very last
            // step of completing the transaction.
            auto new_footer_pos = typed_address<trailer>::null ();
            {
                auto const & head = db.get_header ();
                auto const prev_footer = db.getro (head.footer_pos.load ());

                unsigned const generation = prev_footer->a.generation + 1;

                // Make a copy of the index locations; write out any modifications to the indices.
                // Any updated indices will modify the 'locations' array.
                //
                // This must happen before the transaction is final because we're allocating and
                // writing data here.

                auto locations = prev_footer->a.index_records;
                index::flush_indices (*this, &locations, generation);

                // Writing new data is done. Now we begin to build the new file footer.
                {
                    std::shared_ptr<trailer> trailer_ptr;
                    std::tie (trailer_ptr, new_footer_pos) = this->alloc_rw<trailer> ();
                    auto * const t = new (trailer_ptr.get ()) trailer;

                    t->a.index_records = locations;

                    // Point the new header at the previous version.
                    t->a.generation = generation;
                    // The size of the transaction doesn't include the size of the footer record.
                    t->a.size = size_ - sizeof (trailer);
                    t->a.time = pstore::milliseconds_since_epoch ();
                    t->a.prev_generation = head.footer_pos;
                    t->crc = t->get_crc ();
                }
            }
            // Complete the transaction by making it available to other clients. This modifies the
            // footer pointer in the file's header record.
            db.set_new_footer (new_footer_pos);

            // Mark both this transaction's contents and its trailer as read-only.
            db.protect (first_, (new_footer_pos + 1).to_address ());

            // That's the end of this transaction.
            first_ = address::null ();
            assert (!this->is_open ());
        }
        return *this;
    }

    // rollback
    // ~~~~~~~~
    transaction_base & transaction_base::rollback () noexcept {
        if (this->is_open ()) {
            first_ = address::null ();
            assert (!this->is_open ());
            // if we grew the db, truncate it back
            if (db_.size () > dbsize_) {
                db_.truncate (dbsize_);
                assert (db_.size () == dbsize_);
            }
        }
        return *this;
    }

} // namespace pstore
