//*  _                                  _   _              *
//* | |_ _ __ __ _ _ __  ___  __ _  ___| |_(_) ___  _ __   *
//* | __| '__/ _` | '_ \/ __|/ _` |/ __| __| |/ _ \| '_ \  *
//* | |_| | | (_| | | | \__ \ (_| | (__| |_| | (_) | | | | *
//*  \__|_|  \__,_|_| |_|___/\__,_|\___|\__|_|\___/|_| |_| *
//*                                                        *
//===- lib/core/transaction.cpp -------------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
            , first_{rhs.first_} {
        rhs.first_ = address::null ();
        assert (!rhs.is_open ());
    }

    // getro
    // ~~~~~
    std::shared_ptr<void const> transaction_base::getro (address addr, std::size_t size) {
        return db ().getro (addr, size);
    }
    std::shared_ptr<void const> transaction_base::getro (extent const & ex) {
        return this->getro (ex.addr, ex.size);
    }

    // getrw
    // ~~~~~
    // TODO: raise an exception if addr does not lie within the current transaction.
    auto transaction_base::getrw (address addr, std::size_t size) -> std::shared_ptr<void> {
        assert (addr >= first_ && addr + size <= first_ + size_);
        return db_.getrw (addr, size);
    }

    auto transaction_base::getrw (extent const & ex) -> std::shared_ptr<void> {
        return this->getrw (ex.addr, ex.size);
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
            auto new_footer_pos = address::null ();
            {
                auto head = db.getrw<header> (address::null ());

                auto prev_footer = db.getro<trailer const> (head->footer_pos);

                // Make a copy of the index locations; write out any modifications
                // to the indices. Any updated indices will modify the 'locations'
                // array.
                //
                // This must happen before the transaction is final because we're
                // allocate and writing data here.

                auto locations = prev_footer->a.index_records;
                index::flush_indices (*this, &locations);

                // Writing new data is done. Now we begin to build the new file footer.
                {
                    std::shared_ptr<trailer> trailer_ptr;
                    std::tie (trailer_ptr, new_footer_pos) = this->alloc_rw<trailer> ();
                    auto * const t = new (trailer_ptr.get ()) trailer;

                    t->a.index_records = locations;

                    // Point the new header at the previous version.
                    t->a.generation = prev_footer->a.generation + 1;
                    // The size of the transaction doesn't include the size of the footer record.
                    t->a.size = size_ - sizeof (trailer);
                    t->a.time = pstore::milliseconds_since_epoch ();
                    t->a.prev_generation = head->footer_pos;
                    t->crc = t->get_crc ();
                }

                db.set_new_footer (head.get (), new_footer_pos);
            }

            // Mark both this transaction's contents and its trailer as read-only.
            db.protect (first_, new_footer_pos + sizeof (trailer));

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
            // TODO:
            // If we extended the file and added new memory regions, then we need to undo that
            // here. Perhaps a) not bother at all and do nothing or b) delay until the file
            // itself is closed on the basis that we may need the space for the next transaction.
            first_ = address::null ();
            assert (!this->is_open ());
        }
        return *this;
    }

} // namespace pstore
// eof: lib/core/transaction.cpp
