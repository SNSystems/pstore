//*                            _ _       _   _              *
//*   ___ ___  _ __ ___  _ __ (_) | __ _| |_(_) ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ \| | |/ _` | __| |/ _ \| '_ \  *
//* | (_| (_) | | | | | | |_) | | | (_| | |_| | (_) | | | | *
//*  \___\___/|_| |_| |_| .__/|_|_|\__,_|\__|_|\___/|_| |_| *
//*                     |_|                                 *
//===- include/pstore/mcrepo/compilation.hpp ------------------------------===//
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
#ifndef PSTORE_MCREPO_COMPILATION_HPP
#define PSTORE_MCREPO_COMPILATION_HPP

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

        std::ostream & operator<< (std::ostream & os, linkage_type l);

        //*                    _ _      _   _                            _              *
        //*  __ ___ _ __  _ __(_) |__ _| |_(_)___ _ _    _ __  ___ _ __ | |__  ___ _ _  *
        //* / _/ _ \ '  \| '_ \ | / _` |  _| / _ \ ' \  | '  \/ -_) '  \| '_ \/ -_) '_| *
        //* \__\___/_|_|_| .__/_|_\__,_|\__|_\___/_||_| |_|_|_\___|_|_|_|_.__/\___|_|   *
        //*              |_|                                                            *
        /// \brief Represents an individual symbol in a compilation.
        ///
        /// The compilation member provides the connection between a symbol name, its linkage, and
        /// the fragment which holds the associated data.
        struct compilation_member {
            /// \param d  The fragment digest for this compilation symbol.
            /// \param x  The fragment extent for this compilation symbol.
            /// \param n  Symbol name address.
            /// \param l  The symbol linkage.
            compilation_member (index::digest d, extent<fragment> x,
                                typed_address<indirect_string> n, linkage_type l)
                    : digest{d}
                    , fext{x}
                    , name{n}
                    , linkage{l} {}

            /// The digest of the fragment reference by this compilation symbol.
            index::digest digest;
            /// The extent of the fragment referenced by this compilation symbol.
            extent<fragment> fext;
            typed_address<indirect_string> name;
            linkage_type linkage;
            // TODO: eliminate this padding.
            std::uint8_t padding1 = 0;
            std::uint16_t padding2 = 0;
            std::uint32_t padding3 = 0;

            /// \brief Returns a pointer to an in-store compilation member instance.
            ///
            /// \param db  The database from which the ticket_member should be loaded.
            /// \param addr  Address of the compilation member.
            /// \result  A pointer to the in-store compilation member.
            static std::shared_ptr<compilation_member const>
            load (pstore::database const & db, pstore::typed_address<compilation_member> addr) {
                return db.getro (addr);
            }

        private:
            friend class compilation;
            compilation_member () noexcept = default;
        };

        PSTORE_STATIC_ASSERT (std::is_standard_layout<compilation_member>::value);
        PSTORE_STATIC_ASSERT (alignof (compilation_member) == 16);
        PSTORE_STATIC_ASSERT (sizeof (compilation_member) == 48);
        PSTORE_STATIC_ASSERT (offsetof (compilation_member, digest) == 0);
        PSTORE_STATIC_ASSERT (offsetof (compilation_member, fext) == 16);
        PSTORE_STATIC_ASSERT (offsetof (compilation_member, name) == 32);
        PSTORE_STATIC_ASSERT (offsetof (compilation_member, linkage) == 40);
        PSTORE_STATIC_ASSERT (offsetof (compilation_member, padding1) == 41);
        PSTORE_STATIC_ASSERT (offsetof (compilation_member, padding2) == 42);

        //*                    _ _      _   _           *
        //*  __ ___ _ __  _ __(_) |__ _| |_(_)___ _ _   *
        //* / _/ _ \ '  \| '_ \ | / _` |  _| / _ \ ' \  *
        //* \__\___/_|_|_| .__/_|_\__,_|\__|_\___/_||_| *
        //*              |_|                            *
        /// A compilation is a holder for zero or more `compilation_member` instances. It is the
        /// top-level object representing the result of processing of a transaction unit by the
        /// compiler.
        class compilation {
        public:
            using iterator = compilation_member *;
            using const_iterator = compilation_member const *;

            void operator delete (void * p);

            /// \name Construction
            ///@{

            /// \brief Allocates a new compilation in-store and copy the ticket file path and the
            /// contents of a vector of compilation_members into it.
            ///
            /// \param transaction  The transaction to which the compilation will be appended.
            /// \param path  A ticket file path address in the store.
            /// \param triple  The target-triple associated with this compilation.
            /// \param first_member  The first of a sequence of compilation_member instances. The
            /// range
            ///   defined by \p first_member and \p last_member will be copied into the newly
            ///   allocated compilation.
            /// \param last_member  The end of the range of compilation_member instances.
            /// \result A pair of a pointer and an extent which describes
            ///   the in-store location of the allocated compilation.
            template <typename TransactionType, typename Iterator>
            static extent<compilation> alloc (TransactionType & transaction,
                                              typed_address<indirect_string> path,
                                              typed_address<indirect_string> triple,
                                              Iterator first_member, Iterator last_member);

            /// \brief Returns a pointer to an in-pstore compilation instance.
            ///
            /// \param db  The database from which the compilation should be loaded.
            /// \param extent  An extent describing the compilation location in the store.
            /// \result  A pointer to the compilation in-store memory.
            static std::shared_ptr<compilation const> load (database const & db,
                                                            extent<compilation> const & extent);

            ///@}

            /// \name Element access
            ///@{
            compilation_member const & operator[] (std::size_t i) const {
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

            /// Returns the number of bytes of storage required for a compilation with 'size'
            /// members.
            static std::size_t size_bytes (std::uint64_t size) {
                return sizeof (compilation) - sizeof (compilation::members_) +
                       sizeof (compilation::members_[0]) * size;
            }

            /// \returns The number of bytes needed to accommodate this compilation.
            std::size_t size_bytes () const { return compilation::size_bytes (this->size ()); }
            ///@}

            /// Returns the ticket file path.
            typed_address<indirect_string> path () const { return path_; }
            /// Returns the target triple.
            typed_address<indirect_string> triple () const { return triple_; }

        private:
            compilation () noexcept;

            struct nmembers {
                std::size_t n;
            };
            /// A placement-new implementation which allocates sufficient storage for a
            /// compilation with the number of members given by the size parameter.
            void * operator new (std::size_t s, nmembers size);
            void operator delete (void * p, nmembers size);

            /// The path containing the ticket file when it was created. (Used to guide the garbage
            /// collector's ticket-file search.)
            typed_address<indirect_string> path_;
            /// The target triple for this compilation.
            typed_address<indirect_string> triple_;
            /// The number of entries in the members_ array.
            std::uint64_t size_ = 0;
            std::uint64_t padding1_ = 0;
            compilation_member members_[1];
        };

        PSTORE_STATIC_ASSERT (std::is_standard_layout<compilation>::value);
        PSTORE_STATIC_ASSERT (alignof (compilation) == 16);

        inline compilation::compilation () noexcept {
            (void) padding1_; // silence warning about an unused private class member.
            PSTORE_STATIC_ASSERT (offsetof (compilation, path_) == 0);
            PSTORE_STATIC_ASSERT (offsetof (compilation, triple_) == 8);
            PSTORE_STATIC_ASSERT (offsetof (compilation, size_) == 16);
            PSTORE_STATIC_ASSERT (offsetof (compilation, padding1_) == 24);
            PSTORE_STATIC_ASSERT (offsetof (compilation, members_) == 32);
        }

        // alloc
        // ~~~~~
        template <typename TransactionType, typename Iterator>
        auto compilation::alloc (TransactionType & transaction, typed_address<indirect_string> path,
                                 typed_address<indirect_string> triple, Iterator first_member,
                                 Iterator last_member) -> extent<compilation> {
            // First work out its size.
            auto const dist = std::distance (first_member, last_member);
            assert (dist >= 0);
            auto const num_members = static_cast<std::uint64_t> (dist);
            auto const size = size_bytes (num_members);

            // Allocate the storage.
            auto const addr = transaction.allocate (size, alignof (compilation));
            auto ptr = std::static_pointer_cast<compilation> (transaction.getrw (addr, size));

            // Write the data to the store.
            ptr->path_ = path;
            ptr->triple_ = triple;
            ptr->size_ = num_members;
            std::copy (first_member, last_member, ptr->begin ());
            return pstore::extent<compilation> (typed_address<compilation> (addr), size);
        }

    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_COMPILATION_HPP
