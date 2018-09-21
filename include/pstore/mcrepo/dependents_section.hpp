//*      _                           _            _        *
//*   __| | ___ _ __   ___ _ __   __| | ___ _ __ | |_ ___  *
//*  / _` |/ _ \ '_ \ / _ \ '_ \ / _` |/ _ \ '_ \| __/ __| *
//* | (_| |  __/ |_) |  __/ | | | (_| |  __/ | | | |_\__ \ *
//*  \__,_|\___| .__/ \___|_| |_|\__,_|\___|_| |_|\__|___/ *
//*            |_|                                         *
//*                _   _              *
//*  ___  ___  ___| |_(_) ___  _ __   *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* \__ \  __/ (__| |_| | (_) | | | | *
//* |___/\___|\___|\__|_|\___/|_| |_| *
//*                                   *
//===- include/pstore/mcrepo/dependents_section.hpp -----------------------===//
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
#ifndef PSTORE_MCREPO_DEPENDENTS_SECTION_HPP
#define PSTORE_MCREPO_DEPENDENTS_SECTION_HPP

#include <cstdlib>

#include "pstore/core/address.hpp"
#include "pstore/mcrepo/repo_error.hpp"
#include "pstore/mcrepo/section.hpp"
#include "pstore/mcrepo/ticket.hpp"

namespace pstore {
    namespace repo {

        //*     _                       _         _       *
        //*  __| |___ _ __  ___ _ _  __| |___ _ _| |_ ___ *
        //* / _` / -_) '_ \/ -_) ' \/ _` / -_) ' \  _(_-< *
        //* \__,_\___| .__/\___|_||_\__,_\___|_||_\__/__/ *
        //*          |_|                                  *
        /// This class represents dependent ticket members of a fragment.
        ///
        /// When a new global object (GO) is generated by an LLVM optimisation pass
        /// after the RepoHashGenerator pass, the digest will be generated by the RepoObjectWriter.
        /// If a fragment has an external fixup referencing this GO, we add the GO's ticket member
        /// address to the dependents.
        ///
        /// When a fragment is pruned, all its dependents need to be recorded in the repo.tickets
        /// metadata to guarantee that the dependents (those GO generated by later optimizations)
        /// are present in the compilation's final ticket.
        class dependents {
        public:
            using iterator = typed_address<ticket_member> *;
            using const_iterator = typed_address<ticket_member> const *;

            template <typename Iterator>
            dependents (Iterator begin, Iterator end);

            dependents (dependents const &) = delete;
            dependents & operator= (dependents const &) = delete;

            /// \name Element access
            ///@{

            typed_address<ticket_member> operator[] (std::size_t i) const {
                assert (i < size_);
                return ticket_members_[i];
            }
            typed_address<ticket_member> & operator[] (std::size_t i) {
                assert (i < size_);
                return ticket_members_[i];
            }
            ///@}

            /// \name Iterators
            ///@{

            iterator begin () { return ticket_members_; }
            const_iterator begin () const { return ticket_members_; }
            const_iterator cbegin () const { return this->begin (); }

            iterator end () { return ticket_members_ + size_; }
            const_iterator end () const { return ticket_members_ + size_; }
            const_iterator cend () const { return this->end (); }
            ///@}

            /// \name Capacity
            ///@{

            /// Checks whether the container is empty.
            bool empty () const noexcept { return size_ == 0; }
            /// Returns the number of elements.
            std::size_t size () const noexcept { return size_; }
            ///@}

            /// \name Storage
            ///@{

            /// Returns the number of bytes of storage required for the dependents with
            /// 'size' children.
            static std::size_t size_bytes (std::uint64_t size) noexcept;

            /// Returns the number of bytes of storage required for the dependents.
            std::size_t size_bytes () const noexcept;
            ///@}

            /// \brief Returns a pointer to the dependents instance.
            ///
            /// \param db The database from which the dependents should be loaded.
            /// \param dependent Address of the dependents in the store.
            /// \result A pointer to the dependents in-store memory.
            static auto load (database const & db, typed_address<dependents> const dependent)
                -> std::shared_ptr<dependents const>;

        private:
            std::uint64_t size_;
            typed_address<ticket_member> ticket_members_[1];
        };

        template <typename Iterator>
        dependents::dependents (Iterator begin, Iterator end) {
            static_assert (std::is_same<typename std::iterator_traits<Iterator>::value_type,
                                        typed_address<ticket_member>>::value,
                           "Iterator value_type must be typed_address<ticket_member>");

            auto const size = std::distance (begin, end);
            assert (size >= 0);

            size_ = static_cast<std::uint64_t> (size);
            std::copy (begin, end, &ticket_members_[0]);

            static_assert (std::is_standard_layout<dependents>::value,
                           "dependents must be standard-layout");
            static_assert (offsetof (dependents, size_) == 0,
                           "offsetof (dependents, size_) must be 0");
            static_assert (offsetof (dependents, ticket_members_) == 8,
                           "offset of the first dependent must be 8");
        }


        //*                  _   _               _ _               _      _             *
        //*  __ _ _ ___ __ _| |_(_)___ _ _    __| (_)____ __  __ _| |_ __| |_  ___ _ _  *
        //* / _| '_/ -_) _` |  _| / _ \ ' \  / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_| *
        //* \__|_| \___\__,_|\__|_\___/_||_| \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_|   *
        //*                                            |_|                              *
        class dependents_creation_dispatcher final : public section_creation_dispatcher {
        public:
            using const_iterator = typed_address<ticket_member> const *;

            dependents_creation_dispatcher (const_iterator begin, const_iterator end)
                    : section_creation_dispatcher (section_kind::dependent)
                    , begin_ (begin)
                    , end_ (end) {
                assert (std::distance (begin, end) >= 0);
            }
            dependents_creation_dispatcher (dependents_creation_dispatcher const &) = delete;
            dependents_creation_dispatcher &
            operator= (dependents_creation_dispatcher const &) = delete;

            std::size_t size_bytes () const final;

            std::uint8_t * write (std::uint8_t * out) const final;

            /// Returns a const_iterator for the beginning of the ticket_member address range.
            const_iterator begin () const { return begin_; }
            /// Returns a const_iterator for the end of ticket_member address range.
            const_iterator end () const { return end_; }

        private:
            std::uintptr_t aligned_impl (std::uintptr_t in) const final;

            const_iterator begin_;
            const_iterator end_;
        };


        class dependents_dispatcher final : public dispatcher {
        public:
            explicit dependents_dispatcher (dependents const & d) noexcept
                    : d_{d} {}
            ~dependents_dispatcher () noexcept override;

            std::size_t size_bytes () const final { return d_.size_bytes (); }
            unsigned align () const final { error (); }
            std::size_t size () const final { error (); }
            container<internal_fixup> ifixups () const final { error (); }
            container<external_fixup> xfixups () const final { error (); }
            container<std::uint8_t> data () const final { error (); }

        private:
            PSTORE_NO_RETURN void error () const;
            dependents const & d_;
        };


        template <>
        struct section_to_dispatcher<dependents> {
            using type = dependents_dispatcher;
        };

    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_DEPENDENTS_SECTION_HPP
