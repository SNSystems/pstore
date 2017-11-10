//*  _                                  _   _              *
//* | |_ _ __ __ _ _ __  ___  __ _  ___| |_(_) ___  _ __   *
//* | __| '__/ _` | '_ \/ __|/ _` |/ __| __| |/ _ \| '_ \  *
//* | |_| | | (_| | | | \__ \ (_| | (__| |_| | (_) | | | | *
//*  \__|_|  \__,_|_| |_|___/\__,_|\___|\__|_|\___/|_| |_| *
//*                                                        *
//===- include/pstore/transaction.hpp -------------------------------------===//
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
/// \file transaction.hpp
/// \brief The data store transaction class.

#ifndef PSTORE_TRANSACTION_HPP
#define PSTORE_TRANSACTION_HPP

#include <mutex>
#include <type_traits>

#include "pstore/database.hpp"
#include "pstore/hamt_map.hpp"
#include "pstore/hamt_set.hpp"
#include "pstore/start_vacuum.hpp"
#include "pstore/time.hpp"

namespace pstore {
    /// \brief The database transaction class.
    ///
    /// When a transaction object is instantiated, a transation begins. Every subsequent
    /// operation can be potentially undone if the rollback() method is called. The commit()
    /// method commits the work performed by all operations since the start of the
    /// transaction.
    ///
    /// Similarly, the rollback() method command undoes all of the work performed by all
    /// operations since the start of the transaction. If neither the commit() nor rollback()
    /// methods are called before the object is destroyed, a commit() is performed by the
    /// destructor (unless an exception is being unwound). A transaction is a scope in which
    /// operations are performed together and committed, or completely reversed.

    template <typename LockGuard>
    class transaction {
    public:
        using lock_type = LockGuard;

        transaction (database & db, lock_type && lock);
        virtual ~transaction () noexcept;

        database & db () {
            return db_;
        }

        /// Commits all modifications made to the data store as part of this transaction.
        /// Modifications are visible to other processes when the commit is complete.
        transaction & commit ();

        /// Discards all modifications made to the data store as part of this transaction.
        transaction & rollback () noexcept;

        /// Returns true if data has been added to this transaction, but not yet committed. In
        /// other words, if it returns false, calls to commit() or rollback() are noops.
        bool is_open () const noexcept {
            return first_ != address::null ();
        }

        ///@{
        auto getro (address const & addr, std::size_t size) -> std::shared_ptr<void const>;
        auto getro (record const & r) -> std::shared_ptr<void const>;
        template <typename Ty>
        auto getro (address const & sop) -> std::shared_ptr<Ty const>;
        ///@}


        ///@{
        auto getrw (address const & addr, std::size_t size) -> std::shared_ptr<void>;
        auto getrw (record const & r) -> std::shared_ptr<void>;

        template <typename Ty,
                  typename = typename std::enable_if<std::is_standard_layout<Ty>::value>::type>
        auto getrw (address const & addr, std::size_t elements) -> std::shared_ptr<Ty>;

        template <typename Ty,
                  typename = typename std::enable_if<std::is_standard_layout<Ty>::value>::type>
        auto getrw (address const & addr) -> std::shared_ptr<Ty>;
        ///@}


        ///@{

        /// Allocates sufficient space in the transaction for 'size' bytes
        /// at an alignment given by 'align' and returns both a writable pointer
        /// to the new space and its address.
        ///
        /// \param size  The number of bytes of storage to allocate.
        /// \param align The alignment of the newly allocated storage. Must be a non-zero power
        ///              of two.
        /// \returns  A std::pair which contains a writable pointer to the newly
        ///           allocated space and the address of that space.
        ///
        /// \note     The newly allocated space is not initialized.
        auto alloc_rw (std::size_t size, unsigned align)
            -> std::pair<std::shared_ptr<void>, address> {
            address const addr = this->allocate (size, align);
            // We call database::get() with the initialized parameter set to false because this
            // is new storage: there's no need to copy its existing contents if the block spans
            // more than one region.
            auto ptr = std::const_pointer_cast<void> (db_.get (addr, size,
                                                               false,  // initialized?
                                                               true)); // writable?
            return {ptr, addr};
        }

        /// Allocates sufficient space in the transaction for one or more new instances of
        /// type 'Ty' and returns both a writable pointer to the new space and
        /// its address. Ty must be a "standard layout" type.
        ///
        /// \param num  The number of instances of type Ty for which space should be allocated.
        /// \returns  A std::pair which contains a writable pointer to the newly
        ///           allocated space and the address of that space.
        ///
        /// \note     The newly allocated space it not initialized.
        template <typename Ty,
                  typename = typename std::enable_if<std::is_standard_layout<Ty>::value>::type>
        auto alloc_rw (std::size_t num = 1) -> std::pair<std::shared_ptr<Ty>, address> {
            auto result = this->alloc_rw (sizeof (Ty) * num, alignof (Ty));
            return {std::static_pointer_cast<Ty> (result.first), result.second};
        }
        ///@}

        ///@{
        /// Extend the database store ensuring that there's enough room for the requested number
        /// of bytes with any additional padding to statify the alignment requirement.
        ///
        /// \param size   The number of bytes of storages to be allocated.
        /// \param align  The alignment of the allocated storage. Must be a power of 2.
        /// \result       The database address of the new storage.
        /// \note     The newly allocated space is not initialized.
        virtual auto allocate (std::uint64_t size, unsigned align) -> address;

        /// Extend the database store ensuring that there's enough room for an instance of the
        /// template type.
        /// \result  The database address of the new storage.
        /// \note    The newly allocated space is not initialized.
        template <typename Ty>
        auto allocate () -> address;
        ///@}

        // Move is supported
        transaction (transaction && rhs) noexcept;
        transaction & operator= (transaction && rhs) noexcept;

        // No assignment or copying.
        transaction (transaction const &) = delete;
        transaction & operator= (transaction const &) = delete;

    private:
        /// Write out any indices that have changed. Any that haven't will
        /// continue to point at their previous incarnation. Update the
        /// members of the 'locations' array.
        ///
        /// This happens early in the process of commiting a transaction; we're
        /// allocating and writing space in the store here.
        ///
        /// \param locations  An array of the index locations. Any modified indices
        ///                   will be modified to point at the new file address.
        void flush_indices (trailer::index_records_array * const locations);

        database & db_;
        lock_type lock_;
        /// The number of bytes allocated in this transaction.
        std::uint64_t size_ = 0;

        /// The first address occupied by this transaction. 0 if the transaction
        /// has not yet allocated any data.
        address first_ = address::null ();
    };
} // namespace pstore


namespace pstore {


    /// lock_guard fills a similar role as a type such as std::scoped_lock<> in that it provides
    /// convenient RAII-style mechanism for owning a mutex for the duration of a scoped block. The
    /// major differences are that it manages only a single mutex, and that it assumes ownership of
    /// the mutex.
    template <typename MutexType>
    class lock_guard {
    public:
        explicit lock_guard (MutexType && mut)
                : mut_{std::move (mut)} {
            mut_.lock ();
            owned_ = true;
        }
        lock_guard (lock_guard && rhs) noexcept
                : mut_{std::move (rhs.mut_)} {

            owned_ = rhs.owned_;
            rhs.owned_ = false;
        }

        ~lock_guard () {
            if (owned_) {
                mut_.unlock ();
                owned_ = false;
            }
        }

        lock_guard & operator= (lock_guard && rhs) noexcept {
            if (this != &rhs) {
                mut_ = std::move (rhs.mut_);
                owned_ = rhs.owned_;
                rhs.owned_ = false;
            }
            return *this;
        }

        // No copying or assignment.
        lock_guard & operator= (lock_guard const & rhs) = delete;
        lock_guard (lock_guard const & rhs) = delete;

    private:
        MutexType mut_;
        bool owned_;
    };


    /// A mutex which is used to protect a pstore file from being simultaneously written by multiple threads or processes.
    class transaction_mutex {
    public:
        explicit transaction_mutex (database & db)
                : rl_{db.file (), offsetof (header, footer_pos), sizeof (header::footer_pos),
                      pstore::file::file_base::lock_kind::exclusive_write} {}

        // Move.
        transaction_mutex (transaction_mutex && ) noexcept = default;
        transaction_mutex & operator= (transaction_mutex && ) noexcept = default;

        // No copying or assignment
        transaction_mutex (transaction_mutex const & ) = delete;
        transaction_mutex & operator= (transaction_mutex const & ) = delete;

        void lock () {
            rl_.lock ();
        }
        void unlock () {
            rl_.unlock ();
        }

    private:
        file::range_lock rl_;
    };

    using transaction_lock = lock_guard<transaction_mutex>;


    ///@{
    /// \brief Creates a new transaction.
    /// Every operation performed on a transaction instance can be potentially undone. The
    /// object's commit() method commits the work performed by all operations since the start of
    /// the transaction.
    template <typename LockGuard>
    inline transaction<LockGuard> begin (database & db, LockGuard && lock) {
        return {db, std::move (lock)};
    }

    inline transaction<transaction_lock> begin (database & db) {
        return begin (db, transaction_lock{transaction_mutex{db}});
    }
    ///@}

} // namespace pstore


namespace pstore {

    // (ctor)
    // ~~~~~~
    template <typename LockGuard>
    transaction<LockGuard>::transaction (database & db, LockGuard && lock)
            : db_{db}
            , lock_{std::move (lock)} {

        // First thing that creating a transaction does is update the view
        // to that of the head revision.
        db.sync ();
        assert (!this->is_open ());
    }
    template <typename LockGuard>
    transaction<LockGuard>::transaction (transaction && rhs) noexcept
            : db_ {rhs.db_}
            , lock_ {std::move (rhs.lock_)}
            , first_ {std::move (rhs.first_)} {

        rhs.first_ = address::null ();
        assert (!rhs.is_open ());
    }

    // (dtor)
    // ~~~~~~
    template <typename LockGuard>
    transaction<LockGuard>::~transaction () noexcept {
        static_assert (noexcept (rollback ()), "rollback must be noexcept");
        this->rollback ();
    }

    // operator=
    // ~~~~~~~~~
    template <typename LockGuard>
    auto transaction<LockGuard>::operator= (transaction && rhs) noexcept -> transaction & {
        db_ = std::move (rhs.db_);
        lock_ = std::move (rhs.lock_);
        first_ = std::move (rhs.first_);

        rhs.db_ = nullptr;
        rhs.open_ = address::null ();
        assert (!rhs.is_open ());
        return *this;
    }

    // allocate
    // ~~~~~~~~
    template <typename LockGuard>
    auto transaction<LockGuard>::allocate (std::uint64_t size, unsigned align) -> address {
        std::uint64_t const old_size = db_.size ();
        address result = db_.allocate (lock_, size, align);
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
        auto const bytes_allocated = db_.size () - old_size;
        assert (bytes_allocated >= size);

        size_ += bytes_allocated;
        return result;
    }
    template <typename LockGuard>
    template <typename Ty>
    auto transaction<LockGuard>::allocate () -> address {
        return this->allocate (sizeof (Ty), alignof (Ty));
    }

    // getrw
    // ~~~~~
    // TODO: raise an exception if addr does not lie within the current transaction.
    template <typename LockGuard>
    auto transaction<LockGuard>::getrw (address const & addr, std::size_t size)
        -> std::shared_ptr<void> {
        assert (addr >= first_ && addr + size <= first_ + size_);
        return db_.getrw (addr, size);
    }

    template <typename LockGuard>
    auto transaction<LockGuard>::getrw (record const & r) -> std::shared_ptr<void> {
        return this->getrw (r.addr, r.size);
    }


    template <typename LockGuard>
    template <typename Ty, typename /*enable_if standard_layout*/>
    auto transaction<LockGuard>::getrw (address const & addr, std::size_t elements)
        -> std::shared_ptr<Ty> {
        return std::static_pointer_cast<Ty> (this->getrw (addr, elements * sizeof (Ty)));
    }
    template <typename LockGuard>
    template <typename Ty, typename /*standard_layout*/>
    inline auto transaction<LockGuard>::getrw (address const & addr) -> std::shared_ptr<Ty> {
        return this->getrw<Ty> (addr, std::size_t{1});
    }



    // getro
    // ~~~~~
    template <typename LockGuard>
    auto transaction<LockGuard>::getro (address const & addr, std::size_t size)
        -> std::shared_ptr<void const> {
        return db_.getro (addr, size);
    }
    template <typename LockGuard>
    auto transaction<LockGuard>::getro (record const & r) -> std::shared_ptr<void const> {
        return this->getro (r.addr, r.size);
    }

    template <typename LockGuard>
    template <typename Ty>
    inline auto transaction<LockGuard>::getro (address const & sop) -> std::shared_ptr<Ty const> {
        PSTORE_STATIC_ASSERT (std::is_standard_layout<Ty>::value);
        return db_.getro<Ty const> (sop);
    }


    // commit
    // ~~~~~~
    template <typename LockGuard>
    auto transaction<LockGuard>::commit () -> transaction & {
        if (this->is_open ()) {

            // We're going to write to the header, but this must be the very last
            // step of completing the transaction.
            auto new_footer_pos = address::null ();
            {
                auto head = db_.getrw<header> (address::null ());

                auto prev_footer = db_.getro<trailer const> (head->footer_pos);

                // Make a copy of the index locations; write out any modifications
                // to the indices. Any updated indices will modify the 'locations'
                // array.
                //
                // This must happen before the transaction is final because we're
                // allocate and writing data here.

                auto locations = prev_footer->a.index_records;
                this->flush_indices (&locations);

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

                db_.set_new_footer (head.get (), new_footer_pos);
            }

            // Mark both this transaction's contents and its trailer as read-only.
            db_.protect (first_, new_footer_pos + sizeof (trailer));

            // That's the end of this transaction.
            first_ = address::null ();
            assert (!this->is_open ());
        }
        return *this;
    }

    // rollback
    // ~~~~~~~~
    template <typename LockGuard>
    auto transaction<LockGuard>::rollback () noexcept -> transaction & {
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

    // flush_indices
    // ~~~~~~~~~~~~~
    template <typename LockGuard>
    void transaction<LockGuard>::flush_indices (trailer::index_records_array * const locations) {

        if (index::write_index * const write = db_.get_write_index (false /*create*/)) {
            (*locations)[trailer::indices::write] = write->flush (*this);
        }

        if (index::digest_index * const digest = db_.get_digest_index (false /*create*/)) {
            (*locations)[trailer::indices::digest] = digest->flush (*this);
        }

        if (index::ticket_index * const ticket = db_.get_ticket_index (false /*create*/)) {
            (*locations)[trailer::indices::ticket] = ticket->flush (*this);
        }

        if (index::name_index * const name = db_.get_name_index (false /*create*/)) {
            (*locations)[trailer::indices::name] = name->flush (*this);
        }

        assert (locations->size () == trailer::indices::last);
    }

} // namespace pstore
#endif // PSTORE_TRANSACTION_HPP
// eof: include/pstore/transaction.hpp
