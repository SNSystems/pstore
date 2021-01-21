//*  _                                  _   _              *
//* | |_ _ __ __ _ _ __  ___  __ _  ___| |_(_) ___  _ __   *
//* | __| '__/ _` | '_ \/ __|/ _` |/ __| __| |/ _ \| '_ \  *
//* | |_| | | (_| | | | \__ \ (_| | (__| |_| | (_) | | | | *
//*  \__|_|  \__,_|_| |_|___/\__,_|\___|\__|_|\___/|_| |_| *
//*                                                        *
//===- lib/core/transaction.cpp -------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file transaction.cpp
/// \brief Data store transaction implementation
#include "pstore/core/transaction.hpp"

#include <utility>

#include "pstore/core/index_types.hpp"

namespace pstore {

    // ctor
    // ~~~~
    transaction_base::transaction_base (database & db)
            : db_{db} {
        if (!db.is_writable ()) {
            raise (error_code::transaction_on_read_only_database);
        }

        // First thing that creating a transaction does is update the view
        // to that of the head revision.
        db_.sync ();
        dbsize_ = db.size ();
    }

    transaction_base::transaction_base (transaction_base && rhs) noexcept
            : db_{rhs.db_}
            , size_{rhs.size_}
            , dbsize_{rhs.dbsize_}
            , first_ (rhs.first_) {
        rhs.first_ = address::null ();

        PSTORE_ASSERT (!rhs.is_open ()); //! OCLINT(PH - don't warn about the assert macro)
    }

    // allocate
    // ~~~~~~~~
    address transaction_base::allocate (std::uint64_t const size, unsigned const align) {
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
        PSTORE_ASSERT (bytes_allocated >= size);
        size_ += bytes_allocated;
        return result;
    }

    // alloc_rw
    // ~~~~~~~~
    std::pair<std::shared_ptr<void>, address> transaction_base::alloc_rw (std::size_t const size,
                                                                          unsigned const align) {
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
        if (!this->is_open ()) {
            // No data was added to the transaction. Nothing to do.
            return *this;
        }

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
        PSTORE_ASSERT (!this->is_open ()); //! OCLINT(PH - don't warn about the assert macro)
        return *this;
    }

    // rollback
    // ~~~~~~~~
    transaction_base & transaction_base::rollback () noexcept {
        if (this->is_open ()) {
            first_ = address::null ();
            PSTORE_ASSERT (!this->is_open ()); //! OCLINT(PH - don't warn about the assert macro)
            // if we grew the db, truncate it back
            if (db_.size () > dbsize_) {
                db_.truncate (dbsize_);
                PSTORE_ASSERT (db_.size () == dbsize_);
            }
        }
        return *this;
    }


    // *********
    // * begin *
    // *********
    transaction<transaction_lock> begin (database & db) {
        return begin (db, transaction_lock{transaction_mutex{db}});
    }

} // end namespace pstore
