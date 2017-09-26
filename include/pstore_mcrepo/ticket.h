//*  _   _      _        _    *
//* | |_(_) ___| | _____| |_  *
//* | __| |/ __| |/ / _ \ __| *
//* | |_| | (__|   <  __/ |_  *
//*  \__|_|\___|_|\_\___|\__| *
//*                           *
//===- include/pstore_mcrepo/ticket.h -------------------------------------===//
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
#ifndef PSTORE_MCREPO_TICKET_H
#define PSTORE_MCREPO_TICKET_H

#include "pstore/transaction.hpp"

namespace pstore {
    namespace repo {

#define PSTORE_REPO_LINKAGE_TYPES                                                                  \
    X (ExternalLinkage)                                                                            \
    X (ExternalWeakLinkage)                                                                        \
    X (PrivateLinkage)                                                                             \
    X (InternalLinkage)                                                                            \
    X (AvailableExternallyLinkage)                                                                 \
    X (LinkOnceAnyLinkage)                                                                         \
    X (LinkOnceODRLinkage)                                                                         \
    X (WeakAnyLinkage)                                                                             \
    X (WeakODRLinkage)                                                                             \
    X (AppendingLinkage)                                                                           \
    X (CommonLinkage)

#define X(a) a,
        enum class linkage_type : std::uint8_t { PSTORE_REPO_LINKAGE_TYPES };
#undef X

		//*  _   _    _       _                     _              *
		//* | |_(_)__| |_____| |_   _ __  ___ _ __ | |__  ___ _ _  *
		//* |  _| / _| / / -_)  _| | '  \/ -_) '  \| '_ \/ -_) '_| *
		//*  \__|_\__|_\_\___|\__| |_|_|_\___|_|_|_|_.__/\___|_|   *
		//* 													   *
		struct ticket_member {
            pstore::index::digest digest;
            pstore::address name;
            std::uint8_t linkage;
            bool comdat;
            std::uint16_t padding1;
            std::uint32_t padding2;
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
        static_assert (offsetof (ticket_member, comdat) == 25,
                       "Comdat offset differs from expected value");
        static_assert (offsetof (ticket_member, padding1) == 26,
                       "Padding1 offset differs from expected value");
        static_assert (offsetof (ticket_member, padding2) == 28,
                       "Padding2 offset differs from expected value");

		//*  _   _    _       _    *
		//* | |_(_)__| |_____| |_  *
		//* |  _| / _| / / -_)  _| *
		//*  \__|_\__|_\_\___|\__| *
		//* 					   *
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
            /// \param members  A vector of ticket_member whose contents will be copied into the
            /// newly allocated ticket.
            /// \result A record to the allocated ticket which is in-store.
            template <typename TransactionType>
            static pstore::record alloc (TransactionType & transaction, pstore::address path,
                                         std::vector<ticket_member> const & members);

            /// \brief Returns a pointer to a Ticket which is in-store.
            ///
            /// \param db  The database from which the ticket should be loaded.
            /// \param record  A record to the ticket location in the store.
            /// \result  A pointer to the ticket in-store memory.
            static auto get_ticket (pstore::database const & db, pstore::record const & record)
                -> std::shared_ptr<ticket const>;

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

            iterator begin () {
                return members_;
            }
            const_iterator begin () const {
                return members_;
            }
            const_iterator cbegin () const {
                return this->begin ();
            }

            iterator end () {
                return members_ + size_;
            }
            const_iterator end () const {
                return members_ + size_;
            }
            const_iterator cend () const {
                return this->end ();
            }
            ///@}

            /// \name Capacity
            ///@{

            /// Checks whether the container is empty.
            bool empty () const {
                return size_ == 0;
            }
            /// Returns the number of elements.
            std::size_t size () const {
                return size_;
            }
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
            std::size_t size_bytes () const {
                return ticket::size_bytes (this->size ());
            }
            ///@}

            /// Returns the ticket file path.
            pstore::address path () const {
                return path_addr_;
            }

        private:
            ticket () = default;

            struct nMembers {
                std::size_t n;
            };
            /// A placement-new implementation which allocates sufficient storage for a
            /// ticket with the number of members given by the size parameter.
            void * operator new (std::size_t s, nMembers size);
            void operator delete (void * p, nMembers size);

            pstore::address path_addr_;
            std::uint64_t size_ = 0;
            ticket_member members_[1];
        };

        static_assert (std::is_standard_layout<ticket>::value,
                       "Ticket must satisfy StandardLayoutType");

        // alloc
        // ~~~~~~~~
        template <typename TransactionType>
        pstore::record ticket::alloc (TransactionType & transaction, pstore::address path,
                                      std::vector<ticket_member> const & members) {
            // First work out its size.
            std::uint64_t num_members = members.size ();
            auto size = size_bytes (num_members);

            // Allocate the storage.
            auto addr = transaction.allocate (size, alignof (ticket));
            auto ptr = std::static_pointer_cast<ticket> (transaction.getrw (addr, size));

            // Write the data to the store.
            ptr->path_addr_ = path;
            ptr->size_ = num_members;
            std::copy (members.begin (), members.end (), ptr->begin ());
            return {addr, size};
        }

    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_TICKET_H
// eof: include/pstore_mcrepo/ticket.h
