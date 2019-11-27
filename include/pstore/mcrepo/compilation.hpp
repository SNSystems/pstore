//*                            _ _       _   _              *
//*   ___ ___  _ __ ___  _ __ (_) | __ _| |_(_) ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ \| | |/ _` | __| |/ _ \| '_ \  *
//* | (_| (_) | | | | | | |_) | | | (_| | |_| | (_) | | | | *
//*  \___\___/|_| |_| |_| .__/|_|_|\__,_|\__|_|\___/|_| |_| *
//*                     |_|                                 *
//===- include/pstore/mcrepo/compilation.hpp ------------------------------===//
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
#ifndef PSTORE_MCREPO_COMPILATION_HPP
#define PSTORE_MCREPO_COMPILATION_HPP

#include <new>

#include "pstore/core/index_types.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/mcrepo/repo_error.hpp"
#include "pstore/support/bit_field.hpp"

namespace pstore {
    namespace repo {

#define PSTORE_REPO_LINKAGES                                                                       \
    X (append)                                                                                     \
    X (common)                                                                                     \
    X (external)                                                                                   \
    X (internal_no_symbol)                                                                         \
    X (internal)                                                                                   \
    X (link_once_any)                                                                              \
    X (link_once_odr)                                                                              \
    X (weak_any)                                                                                   \
    X (weak_odr)

#define X(a) a,
        enum class linkage : std::uint8_t { PSTORE_REPO_LINKAGES };
#undef X

        std::ostream & operator<< (std::ostream & os, linkage l);

#define PSTORE_REPO_VISIBILITIES                                                                   \
    X (default_vis)                                                                                \
    X (hidden_vis)                                                                                 \
    X (protected_vis)

#define X(a) a,
        enum class visibility : std::uint8_t { PSTORE_REPO_VISIBILITIES };
#undef X

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
            /// \param v  The symbol visibility.
            compilation_member (index::digest d, extent<fragment> x,
                                typed_address<indirect_string> n, linkage l,
                                visibility v = repo::visibility::default_vis) noexcept;
            /// The digest of the fragment referenced by this compilation symbol.
            index::digest digest;
            /// The extent of the fragment referenced by this compilation symbol.
            extent<fragment> fext;
            //  TODO: it looks tempting to use some of the bits of this address for the
            //  linkage/visibility fields. We know that they're not all used and it would eliminate
            //  all of the padding bits from the structure. Unfortunately, repo-object-writer is
            //  currently stashing host pointers in this field and although the same may be true for
            //  those, it's difficult to be certain.
            typed_address<indirect_string> name;
            union {
                std::uint8_t bf;
                pstore::bit_field<std::uint8_t, 0, 4> linkage_;
                pstore::bit_field<std::uint8_t, 4, 2> visibility_;
            };
            std::uint8_t padding1 = 0;
            std::uint16_t padding2 = 0;
            std::uint32_t padding3 = 0;

            auto linkage () const noexcept -> enum linkage {
                return static_cast<enum linkage> (linkage_.value ());
            }
            auto visibility () const noexcept -> enum visibility {
                return static_cast<enum visibility> (visibility_.value ());
            }

            /// \brief Returns a pointer to an in-store compilation member instance.
            ///
            /// \param db  The database from which the ticket_member should be loaded.
            /// \param addr  Address of the compilation member.
            /// \result  A pointer to the in-store compilation member.
            static auto load (database const & db, typed_address<compilation_member> addr)
                -> std::shared_ptr<compilation_member const>;

        private:
            friend class compilation;
            compilation_member () noexcept = default;
        };
        // load
        // ~~~~
        inline auto compilation_member::load (database const & db,
                                              typed_address<compilation_member> addr)
            -> std::shared_ptr<compilation_member const> {
            return db.getro (addr);
        }

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
            using size_type = std::uint32_t;

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
            bool empty () const noexcept { return size_ == 0; }
            /// Returns the number of elements.
            size_type size () const noexcept { return size_; }
            ///@}

            /// \name Storage
            ///@{

            /// Returns the number of bytes of storage required for a compilation with 'size'
            /// members.
            static std::size_t size_bytes (size_type size) {
                size = std::max (size, size_type{1}); // Always at least enough for one member.
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
            template <typename Iterator>
            compilation (typed_address<indirect_string> path, typed_address<indirect_string> triple,
                         size_type size, Iterator first_member, Iterator last_member) noexcept;

            struct nmembers {
                size_type n;
            };
            /// A placement-new implementation which allocates sufficient storage for a
            /// compilation with the number of members given by the size parameter.
            void * operator new (std::size_t s, nmembers size);
            /// A copy of the standard placement-new function.
            void * operator new (std::size_t s, void * ptr);
            void operator delete (void * p, nmembers size);
            void operator delete (void * p, void * ptr);

            static constexpr std::array<char, 8> compilation_signature_ = {
                {'C', 'm', 'p', 'l', '8', 'i', 'o', 'n'}};

            std::array<char, 8> signature_;

            /// The path containing the ticket file when it was created. (Used to guide the garbage
            /// collector's ticket-file search.)
            typed_address<indirect_string> path_;
            /// The target triple for this compilation.
            typed_address<indirect_string> triple_;
            /// The number of entries in the members_ array.
            size_type size_ = 0;
            std::uint32_t padding1_ = 0;
            compilation_member members_[1];
        };
        PSTORE_STATIC_ASSERT (std::is_standard_layout<compilation>::value);
        PSTORE_STATIC_ASSERT (sizeof (compilation) == 32 + sizeof (compilation_member));
        PSTORE_STATIC_ASSERT (alignof (compilation) == 16);

        template <typename Iterator>
        compilation::compilation (typed_address<indirect_string> path,
                                  typed_address<indirect_string> triple, size_type size,
                                  Iterator first_member, Iterator last_member) noexcept
                : signature_{compilation_signature_}
                , path_{path}
                , triple_{triple}
                , size_{size} {
            // An assignment to suppress a warning from clang that the field is not used.
            padding1_ = 0;
            PSTORE_STATIC_ASSERT (offsetof (compilation, signature_) == 0);
            PSTORE_STATIC_ASSERT (offsetof (compilation, path_) == 8);
            PSTORE_STATIC_ASSERT (offsetof (compilation, triple_) == 16);
            PSTORE_STATIC_ASSERT (offsetof (compilation, size_) == 24);
            PSTORE_STATIC_ASSERT (offsetof (compilation, padding1_) == 28);
            PSTORE_STATIC_ASSERT (offsetof (compilation, members_) == 32);

#ifndef NDEBUG
            {
                // This check can safely be an assertion because the method is private and alloc(),
                // the sole caller, performs a full run-time check of the size.
                auto const dist = std::distance (first_member, last_member);
                assert (dist >= 0 &&
                        static_cast<typename std::make_unsigned<decltype (dist)>::type> (dist) ==
                            size);
            }
#endif
            std::copy (first_member, last_member, this->begin ());
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

            if (dist > std::numeric_limits<size_type>::max ()) {
                raise (error_code::too_many_members_in_compilation);
            }

            auto const num_members = static_cast<size_type> (dist);
            auto const size = size_bytes (num_members);

            // Allocate the storage.
            auto const addr = transaction.allocate (size, alignof (compilation));
            auto ptr = std::static_pointer_cast<compilation> (transaction.getrw (addr, size));

            // Write the data to the store.
            new (ptr.get ()) compilation{path, triple, num_members, first_member, last_member};
            return pstore::extent<compilation> (typed_address<compilation> (addr), size);
        }

    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_COMPILATION_HPP
