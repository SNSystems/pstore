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

#include "pstore/address.hpp"
#include "pstore/database.hpp"
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

    class transaction_base {
    public:
        virtual ~transaction_base () noexcept {}

        transaction_base (transaction_base const &) = delete;
        transaction_base & operator= (transaction_base const &) = delete;
        transaction_base (transaction_base && rhs) noexcept;
        transaction_base & operator= (transaction_base && rhs) noexcept = delete;

        database & db () noexcept {
            return db_;
        }
        database const & db () const noexcept {
            return db_;
        }

        /// Returns true if data has been added to this transaction, but not yet committed. In
        /// other words, if it returns false, calls to commit() or rollback() are noops.
        bool is_open () const noexcept {
            return first_ != address::null ();
        }

        /// Commits all modifications made to the data store as part of this transaction.
        /// Modifications are visible to other processes when the commit is complete.
        transaction_base & commit ();

        /// Discards all modifications made to the data store as part of this transaction.
        transaction_base & rollback () noexcept;


        ///@{
        auto getro (address addr, std::size_t size) -> std::shared_ptr<void const>;
        auto getro (record const & r) -> std::shared_ptr<void const>;
        template <typename Ty>
        auto getro (address addr) -> std::shared_ptr<Ty const>;
        ///@}


        ///@{
        std::shared_ptr<void> getrw (address addr, std::size_t size);
        std::shared_ptr<void> getrw (record const & r);

        template <typename Ty,
                  typename = typename std::enable_if<std::is_standard_layout<Ty>::value>::type>
        std::shared_ptr<Ty> getrw (address addr, std::size_t elements);

        template <typename Ty,
                  typename = typename std::enable_if<std::is_standard_layout<Ty>::value>::type>
        std::shared_ptr<Ty> getrw (address addr);
        ///@}

        ///@{
        /// Extend the database store ensuring that there's enough room for the requested number
        /// of bytes with any additional padding to statify the alignment requirement.
        ///
        /// \param size   The number of bytes of storages to be allocated.
        /// \param align  The alignment of the allocated storage. Must be a power of 2.
        /// \result       The database address of the new storage.
        /// \note     The newly allocated space is not initialized.
        virtual address allocate (std::uint64_t size, unsigned align);

        /// Extend the database store ensuring that there's enough room for an instance of the
        /// template type.
        /// \result  The database address of the new storage.
        /// \note    The newly allocated space is not initialized.
        template <typename Ty>
        address allocate () {
            return this->allocate (sizeof (Ty), alignof (Ty));
        }
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
        std::pair<std::shared_ptr<void>, address> alloc_rw (std::size_t size, unsigned align);

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

    protected:
        transaction_base (database & db)
                : db_{db} {
            // First thing that creating a transaction does is update the view
            // to that of the head revision.
            db_.sync ();
        }

    private:
        database & db_;
        /// The number of bytes allocated in this transaction.
        std::uint64_t size_ = 0;

        /// The first address occupied by this transaction. 0 if the transaction
        /// has not yet allocated any data.
        address first_ = address::null ();
    };


    // getrw
    // ~~~~~
    // TODO: raise an exception if addr does not lie within the current transaction.
    template <typename Ty, typename /*enable_if standard_layout*/>
    inline std::shared_ptr<Ty> transaction_base::getrw (address addr, std::size_t elements) {
        return std::static_pointer_cast<Ty> (this->getrw (addr, elements * sizeof (Ty)));
    }
    template <typename Ty, typename /*standard_layout*/>
    inline std::shared_ptr<Ty> transaction_base::getrw (address addr) {
        return this->getrw<Ty> (addr, std::size_t{1});
    }

    // getro
    // ~~~~~
    template <typename Ty>
    inline std::shared_ptr<Ty const> transaction_base::getro (address addr) {
        PSTORE_STATIC_ASSERT (std::is_standard_layout<Ty>::value); // FIXME: enable_if instead
        return db ().template getro<Ty const> (addr);
    }


    template <typename LockGuard>
    class transaction : public transaction_base {
    public:
        using lock_type = LockGuard;

        transaction (database & db, lock_type && lock);
        ~transaction () noexcept override;

        transaction (transaction && rhs) noexcept = default;
        transaction & operator= (transaction && rhs) noexcept = delete;
        transaction (transaction const &) = delete;
        transaction & operator= (transaction const &) = delete;

    private:
        lock_type lock_;
    };

    // (ctor)
    // ~~~~~~
    template <typename LockGuard>
    transaction<LockGuard>::transaction (database & db, LockGuard && lock)
            : transaction_base (db)
            , lock_{std::move (lock)} {

        assert (!this->is_open ());
    }

    // (dtor)
    // ~~~~~~
    template <typename LockGuard>
    transaction<LockGuard>::~transaction () noexcept {
        static_assert (noexcept (rollback ()), "rollback must be noexcept");
        this->rollback ();
    }
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


    /// A mutex which is used to protect a pstore file from being simultaneously written by multiple
    /// threads or processes.
    class transaction_mutex {
    public:
        explicit transaction_mutex (database & db)
                : rl_{db.file (), offsetof (header, footer_pos), sizeof (header::footer_pos),
                      pstore::file::file_base::lock_kind::exclusive_write} {}

        // Move.
        transaction_mutex (transaction_mutex &&) noexcept = default;
        transaction_mutex & operator= (transaction_mutex &&) noexcept = default;
        // No copying or assignment
        transaction_mutex (transaction_mutex const &) = delete;
        transaction_mutex & operator= (transaction_mutex const &) = delete;

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

#endif // PSTORE_TRANSACTION_HPP
// eof: include/pstore/transaction.hpp
