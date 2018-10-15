//*  _   _      _        _    *
//* | |_(_) ___| | _____| |_  *
//* | __| |/ __| |/ / _ \ __| *
//* | |_| | (__|   <  __/ |_  *
//*  \__|_|\___|_|\_\___|\__| *
//*                           *
//===- include/pstore/mcrepo/ticket.hpp -----------------------------------===//
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
#ifndef PSTORE_MCREPO_TICKET_HPP
#define PSTORE_MCREPO_TICKET_HPP

#include "pstore/core/index_types.hpp"
#include "pstore/core/transaction.hpp"

namespace pstore {
    namespace repo {

#define PSTORE_REPO_LINKAGE_TYPES                                                                  \
    X (append)                                                                                     \
    X (common)                                                                                     \
    X (external)                                                                                   \
    X (internal)                                                                                   \
    X (linkonce)

#define X(a) a,
        enum class linkage_type : std::uint8_t { PSTORE_REPO_LINKAGE_TYPES };
#undef X

        //*  _   _    _       _                     _              *
        //* | |_(_)__| |_____| |_   _ __  ___ _ __ | |__  ___ _ _  *
        //* |  _| / _| / / -_)  _| | '  \/ -_) '  \| '_ \/ -_) '_| *
        //*  \__|_\__|_\_\___|\__| |_|_|_\___|_|_|_|_.__/\___|_|   *
        //*                                                        *
        /// \brief Represents an individual element in a ticket.
        /// The ticket member provides the connection between a symbol name, its linkage, and
        /// the fragment which holds the associated data.
        struct ticket_member {
            /// \param d  The fragment digest for this ticket.
            /// \param n  Symbol name address.
            /// \param l  The symbol linkage.
            ticket_member (index::digest d, typed_address<indirect_string> n, linkage_type l)
                    : digest (d)
                    , name (n)
                    , linkage (l) {}
            index::digest digest;
            typed_address<indirect_string> name;
            linkage_type linkage;
            std::uint16_t padding1 = 0;
            std::uint32_t padding2 = 0;

            /// \brief Returns a pointer to the ticket_member which is in-store.
            ///
            /// \param db The database from which the ticket_member should be loaded.
            /// \param addr Address of the ticket_member in the store.
            /// \result a pointer to the ticket_member in-store memory.
            static std::shared_ptr<ticket_member const>
            load (pstore::database const & db, pstore::typed_address<ticket_member> addr) {
                return db.getro (addr);
            }
        };

        static_assert (std::is_standard_layout<ticket_member>::value,
                       "ticket_member must satisfy StandardLayoutType");
        static_assert (offsetof (ticket_member, digest) == 0,
                       "Digest offset differs from expected value");
        static_assert (sizeof (ticket_member) == 32, "ticket_member size does not match expected");
        static_assert (offsetof (ticket_member, name) == 16,
                       "Name offset differs from expected value");
        static_assert (offsetof (ticket_member, linkage) == 24,
                       "Linkage offset differs from expected value");
        static_assert (offsetof (ticket_member, padding1) == 26,
                       "Padding1 offset differs from expected value");
        static_assert (offsetof (ticket_member, padding2) == 28,
                       "Padding2 offset differs from expected value");

        //*  _   _    _       _    *
        //* | |_(_)__| |_____| |_  *
        //* |  _| / _| / / -_)  _| *
        //*  \__|_\__|_\_\___|\__| *
        //*                        *
        /// \brief A ticket is a holder for zero or more `ticket_member` instances.
        class ticket {
        public:
            using iterator = ticket_member *;
            using const_iterator = ticket_member const *;

            void operator delete (void * p);

            /// \name Construction
            ///@{

            /// \brief Allocates a new Ticket in-store and copy the ticket file path and the
            /// contents of a vector of ticket_members into it.
            ///
            /// \param transaction  The transaction to which the ticket will be appended.
            /// \param path  A ticket file path address in the store.
            /// \param first_member  The first of a sequence of ticket_member instances. The range
            ///   defined by \p first_member and \p last_member will be copied into the newly
            ///   allocated ticket.
            /// \param last_member  The end of the range of ticket_member instances.
            /// \result A pair of a pointer and an extent which describes
            ///   the in-store location of the allocated ticket.
            template <typename TransactionType, typename Iterator>
            static extent<ticket> alloc (TransactionType & transaction,
                                         typed_address<indirect_string> path, Iterator first_member,
                                         Iterator last_member);

            /// \brief Returns a pointer to an in-pstore ticket instance.
            ///
            /// \param db  The database from which the ticket should be loaded.
            /// \param extent  An extent describing the ticket location in the store.
            /// \result  A pointer to the ticket in-store memory.
            static std::shared_ptr<ticket const> load (database const & db,
                                                       extent<ticket> const & extent);

            ///@}

            /// \name Element access
            ///@{
            ticket_member const & operator[] (std::size_t i) const {
                assert (i < size_);
                return members_[i];
            }
            ///@}

            /// \name Iterators
            ///@{

            iterator begin () { return members_; }
            const_iterator begin () const { return members_; }
            const_iterator cbegin () const { return this->begin (); }

            iterator end () { return members_ + size_; }
            const_iterator end () const { return members_ + size_; }
            const_iterator cend () const { return this->end (); }
            ///@}

            /// \name Capacity
            ///@{

            /// Checks whether the container is empty.
            bool empty () const { return size_ == 0; }
            /// Returns the number of elements.
            std::size_t size () const { return size_; }
            ///@}

            /// \name Storage
            ///@{

            /// Returns the number of bytes of storage required for a TicketContent with 'size'
            /// members.
            static std::size_t size_bytes (std::uint64_t size) {
                return sizeof (ticket) - sizeof (ticket::members_) +
                       sizeof (ticket::members_[0]) * size;
            }

            /// \returns The number of bytes needed to accommodate this ticket.
            std::size_t size_bytes () const { return ticket::size_bytes (this->size ()); }
            ///@}

            /// Returns the ticket file path.
            typed_address<indirect_string> path () const { return path_addr_; }

        private:
            ticket () = default;

            struct nMembers {
                std::size_t n;
            };
            /// A placement-new implementation which allocates sufficient storage for a
            /// ticket with the number of members given by the size parameter.
            void * operator new (std::size_t s, nMembers size);
            void operator delete (void * p, nMembers size);

            typed_address<indirect_string> path_addr_;
            std::uint64_t size_ = 0;
            ticket_member members_[1];
        };

        static_assert (std::is_standard_layout<ticket>::value,
                       "Ticket must satisfy StandardLayoutType");

        // alloc
        // ~~~~~
        template <typename TransactionType, typename Iterator>
        auto ticket::alloc (TransactionType & transaction, typed_address<indirect_string> path,
                            Iterator first_member, Iterator last_member) -> extent<ticket> {
            // First work out its size.
            auto const dist = std::distance (first_member, last_member);
            assert (dist >= 0);
            auto const num_members = static_cast<std::uint64_t> (dist);
            auto const size = size_bytes (num_members);

            // Allocate the storage.
            auto const addr = transaction.allocate (size, alignof (ticket));
            auto ptr = std::static_pointer_cast<ticket> (transaction.getrw (addr, size));

            // Write the data to the store.
            ptr->path_addr_ = path;
            ptr->size_ = num_members;
            std::copy (first_member, last_member, ptr->begin ());
            return pstore::extent<ticket> (typed_address<ticket> (addr), size);
        }

    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_TICKET_HPP
