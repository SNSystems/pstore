//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===- include/pstore/mcrepo/fragment.hpp ---------------------------------===//
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
#ifndef PSTORE_MCREPO_FRAGMENT_HPP
#define PSTORE_MCREPO_FRAGMENT_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <type_traits>
#include <vector>

#include "pstore/core/address.hpp"
#include "pstore/core/file_header.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/mcrepo/repo_error.hpp"
#include "pstore/mcrepo/sparse_array.hpp"
#include "pstore/mcrepo/ticket.hpp"
#include "pstore/support/aligned.hpp"
#include "pstore/support/max.hpp"
#include "pstore/support/small_vector.hpp"

namespace pstore {
    class indirect_string;

    namespace repo {

        // TODO: the members of this collection are drawn from
        // RepoObjectWriter::writeRepoSectionData(). It's missing at least the debugging, and
        // EH-related sections and probably others...
        enum class section_kind : std::uint8_t {
            text,
            bss,
            data,
            rel_ro,
            mergeable_1_byte_c_string,
            mergeable_2_byte_c_string,
            mergeable_4_byte_c_string,
            mergeable_const_4,
            mergeable_const_8,
            mergeable_const_16,
            mergeable_const_32,
            read_only,
            thread_bss,
            thread_data,

            // repo metadata sections...
            dependent,
            last // always last, never used.
        };

        constexpr auto first_repo_metadata_section = section_kind::dependent;

        inline bool is_target_section (section_kind t) noexcept {
            using utype = std::underlying_type<section_kind>::type;
            return static_cast<utype> (t) < static_cast<utype> (first_repo_metadata_section);
        }

        using relocation_type = std::uint8_t;

        //*  _     _                     _    __ _                *
        //* (_)_ _| |_ ___ _ _ _ _  __ _| |  / _(_)_ ___  _ _ __  *
        //* | | ' \  _/ -_) '_| ' \/ _` | | |  _| \ \ / || | '_ \ *
        //* |_|_||_\__\___|_| |_||_\__,_|_| |_| |_/_\_\\_,_| .__/ *
        //*                                                |_|    *
        struct internal_fixup {
            internal_fixup (section_kind section_, relocation_type type_, std::uint64_t offset_,
                            std::uint64_t addend_) noexcept
                    : section{section_}
                    , type{type_}
                    , offset{offset_}
                    , addend{addend_} {}
            internal_fixup (internal_fixup const &) noexcept = default;
            internal_fixup (internal_fixup &&) noexcept = default;
            internal_fixup & operator= (internal_fixup const &) noexcept = default;
            internal_fixup & operator= (internal_fixup &&) noexcept = default;

            bool operator== (internal_fixup const & rhs) const noexcept {
                return section == rhs.section && type == rhs.type && offset == rhs.offset &&
                       addend == rhs.addend;
            }
            bool operator!= (internal_fixup const & rhs) const noexcept {
                return !operator== (rhs);
            }

            section_kind section;
            relocation_type type;
            std::uint16_t padding1 = 0;
            std::uint32_t padding2 = 0;
            std::uint64_t offset;
            std::uint64_t addend;
        };


        static_assert (std::is_standard_layout<internal_fixup>::value,
                       "internal_fixup must satisfy StandardLayoutType");

        static_assert (offsetof (internal_fixup, section) == 0,
                       "section offset differs from expected value");
        static_assert (offsetof (internal_fixup, type) == 1,
                       "type offset differs from expected value");
        static_assert (offsetof (internal_fixup, padding1) == 2,
                       "padding1 offset differs from expected value");
        static_assert (offsetof (internal_fixup, padding2) == 4,
                       "padding2 offset differs from expected value");
        static_assert (offsetof (internal_fixup, offset) == 8,
                       "offset offset differs from expected value");
        static_assert (offsetof (internal_fixup, addend) == 16,
                       "addend offset differs from expected value");
        static_assert (sizeof (internal_fixup) == 24,
                       "internal_fixup size does not match expected");

        //*          _                     _    __ _                *
        //*  _____ _| |_ ___ _ _ _ _  __ _| |  / _(_)_ ___  _ _ __  *
        //* / -_) \ /  _/ -_) '_| ' \/ _` | | |  _| \ \ / || | '_ \ *
        //* \___/_\_\\__\___|_| |_||_\__,_|_| |_| |_/_\_\\_,_| .__/ *
        //*                                                  |_|    *
        struct external_fixup {
            typed_address<indirect_string> name;
            relocation_type type;
            // FIXME: much padding here.
            std::uint64_t offset;
            std::uint64_t addend;

            bool operator== (external_fixup const & rhs) const noexcept {
                return name == rhs.name && type == rhs.type && offset == rhs.offset &&
                       addend == rhs.addend;
            }
            bool operator!= (external_fixup const & rhs) const noexcept {
                return !operator== (rhs);
            }
        };

        static_assert (std::is_standard_layout<external_fixup>::value,
                       "external_fixup must satisfy StandardLayoutType");
        static_assert (offsetof (external_fixup, name) == 0,
                       "name offset differs from expected value");
        static_assert (offsetof (external_fixup, type) == 8,
                       "type offset differs from expected value");
        static_assert (offsetof (external_fixup, offset) == 16,
                       "offset offset differs from expected value");
        static_assert (offsetof (external_fixup, addend) == 24,
                       "addend offset differs from expected value");
        static_assert (sizeof (external_fixup) == 32,
                       "external_fixup size does not match expected");



        //*             _   _           *
        //*  ___ ___ __| |_(_)___ _ _   *
        //* (_-</ -_) _|  _| / _ \ ' \  *
        //* /__/\___\__|\__|_\___/_||_| *
        //*                             *
        class section {
        public:
            /// Describes the three members of a section as three pairs of iterators: one
            /// each for the data, internal fixups, and external fixups ranges.
            template <typename DataRangeType, typename IFixupRangeType, typename XFixupRangeType>
            struct sources {
                DataRangeType data_range;
                IFixupRangeType ifixups_range;
                XFixupRangeType xfixups_range;
            };

            template <typename DataRange, typename IFixupRange, typename XFixupRange>
            static inline auto make_sources (DataRange const & d, IFixupRange const & i,
                                             XFixupRange const & x)
                -> sources<DataRange, IFixupRange, XFixupRange> {
                return {d, i, x};
            }

            template <typename DataRange, typename IFixupRange, typename XFixupRange>
            section (DataRange const & d, IFixupRange const & i, XFixupRange const & x,
                     std::uint8_t align);

            template <typename DataRange, typename IFixupRange, typename XFixupRange>
            section (sources<DataRange, IFixupRange, XFixupRange> const & src, std::uint8_t align)
                    : section (src.data_range, src.ifixups_range, src.xfixups_range, align) {}

            section (section const &) = delete;
            section & operator= (section const &) = delete;
            section (section &&) = delete;
            section & operator= (section &&) = delete;

            /// A simple wrapper around the elements of one of the three arrays that make
            /// up a section. Enables the use of standard algorithms as well as
            /// range-based for loops on these collections.
            template <typename ValueType>
            class container {
            public:
                using value_type = ValueType const;
                using size_type = std::size_t;
                using difference_type = std::ptrdiff_t;
                using reference = ValueType const &;
                using const_reference = reference;
                using pointer = ValueType const *;
                using const_pointer = pointer;
                using iterator = const_pointer;
                using const_iterator = iterator;

                container (const_pointer begin, const_pointer end)
                        : begin_{begin}
                        , end_{end} {
                    assert (end >= begin);
                }
                iterator begin () const { return begin_; }
                iterator end () const { return end_; }
                const_iterator cbegin () const { return begin (); }
                const_iterator cend () const { return end (); }

                const_pointer data () const { return begin_; }

                size_type size () const {
                    assert (end_ >= begin_);
                    return static_cast<size_type> (end_ - begin_);
                }

            private:
                const_pointer begin_;
                const_pointer end_;
            };

            unsigned align () const noexcept { return 1U << align_; }

            container<std::uint8_t> data () const {
                auto begin = aligned_ptr<std::uint8_t> (this + 1);
                return {begin, begin + data_size_};
            }
            container<internal_fixup> ifixups () const {
                auto begin = aligned_ptr<internal_fixup> (data ().end ());
                return {begin, begin + this->num_ifixups ()};
            }
            container<external_fixup> xfixups () const {
                auto begin = aligned_ptr<external_fixup> (ifixups ().end ());
                return {begin, begin + num_xfixups_};
            }

            ///@{
            /// \brief A group of member functions which return the number of bytes
            /// occupied by a fragment instance.

            /// \returns The number of bytes occupied by this fragment section.
            std::size_t size_bytes () const;

            /// \returns The number of bytes needed to accommodate a fragment section with
            /// the given number of data bytes and fixups.
            static std::size_t size_bytes (std::size_t data_size, std::size_t num_ifixups,
                                           std::size_t num_xfixups);

            template <typename DataRange, typename IFixupRange, typename XFixupRange>
            static std::size_t size_bytes (DataRange const & d, IFixupRange const & i,
                                           XFixupRange const & x);

            template <typename DataRange, typename IFixupRange, typename XFixupRange>
            static std::size_t
            size_bytes (sources<DataRange, IFixupRange, XFixupRange> const & src) {
                return size_bytes (src.data_range, src.ifixups_range, src.xfixups_range);
            }
            ///@}

        private:
            /// The alignment of this section expressed as a power of two (i.e. 8 byte alignment is
            /// expressed as an align_ value of 3).
            std::uint8_t align_;
            /// The number of internal fixups, stored as a three byte integer (see
            /// `section::three_byte_integer`).
            std::uint8_t num_ifixups_[3];
            /// The number of external fixups in this section.
            std::uint32_t num_xfixups_ = 0;
            /// The number of data bytes contained by this section.
            std::uint64_t data_size_ = 0;

            /// A simple helper type which implements an integer in 3 bytes storing those bytes
            /// in the machine's native integer representation (big- or little-endian).
            class three_byte_integer {
            public:
                /// Reads a three byte integer from the memory at `src`.
                /// \param src  The address of three bytes of memory containing a three-byte
                /// integer.
                /// \result  The value of the integer contained in the memory at `src`.
                static std::uint32_t get (std::uint8_t const * src) noexcept;
                /// Sets the three bytes in the memory at `out` to the value `v`.
                /// \param out  A 3-byte region of memory which will receive the value of `v`.
                /// \param v  The value to be stored.
                static void set (std::uint8_t * out, std::uint32_t v) noexcept;

            private:
                union number {
                    std::uint32_t value;
                    std::uint8_t bytes[sizeof (std::uint32_t)];
                };
            };

            std::uint32_t num_ifixups () const noexcept;
            template <typename Iterator>
            static void set_num_ifixups (Iterator first, Iterator last, std::uint8_t * out);

            /// A helper function which returns the distance between two iterators,
            /// clamped to the maximum range of IntType.
            template <typename IntType, typename Iterator>
            static IntType set_size (Iterator first, Iterator last);

            /// Calculates the size of a region in the section including any necessary
            /// preceeding alignment bytes.
            /// \param pos  The starting offset within the section.
            /// \param num  The number of instance of type Ty.
            /// \returns Number of bytes occupied by the elements.
            template <typename Ty>
            static inline std::size_t part_size_bytes (std::size_t pos, std::size_t num) {
                if (num > 0) {
                    pos = aligned<Ty> (pos) + num * sizeof (Ty);
                }
                return pos;
            }
        };

        // (ctor)
        // ~~~~~~
        template <typename DataRange, typename IFixupRange, typename XFixupRange>
        section::section (DataRange const & d, IFixupRange const & i, XFixupRange const & x,
                          std::uint8_t align)
                : align_{static_cast<std::uint8_t> (bit_count::ctz (align))}
                , num_ifixups_{0} {

            static_assert (std::is_standard_layout<section>::value,
                           "section must satisfy StandardLayoutType");
            static_assert (offsetof (section, align_) == 0, "align_ offset is not 0");
            static_assert (offsetof (section, num_ifixups_) == 1, "num_ifixups_ offset is not 1");
            static_assert (offsetof (section, num_xfixups_) == 4,
                           "num_xfixups_ offset differs from expected value");
            static_assert (offsetof (section, data_size_) == 8,
                           "data_size_ offset differs from expected value");
            static_assert (sizeof (section) == 16, "section size does not match expected");
#ifndef NDEBUG
            auto const start = reinterpret_cast<std::uint8_t *> (this);
#endif
            auto p = reinterpret_cast<std::uint8_t *> (this + 1);
            assert (bit_count::pop_count (align) == 1);

            if (d.first != d.second) {
                p = std::copy (d.first, d.second, aligned_ptr<std::uint8_t> (p));
                data_size_ = section::set_size<decltype (data_size_)> (d.first, d.second);
            }
            if (i.first != i.second) {
                p = reinterpret_cast<std::uint8_t *> (
                    std::copy (i.first, i.second, aligned_ptr<internal_fixup> (p)));
                this->set_num_ifixups (i.first, i.second, &num_ifixups_[0]);
            }
            if (x.first != x.second) {
                p = reinterpret_cast<std::uint8_t *> (
                    std::copy (x.first, x.second, aligned_ptr<external_fixup> (p)));
                num_xfixups_ = section::set_size<decltype (num_xfixups_)> (x.first, x.second);
            }
            assert (p >= start && static_cast<std::size_t> (p - start) == size_bytes (d, i, x));
        }

        // set_size
        // ~~~~~~~~
        template <typename IntType, typename Iterator>
        inline IntType section::set_size (Iterator first, Iterator last) {
            static_assert (std::is_unsigned<IntType>::value, "IntType must be unsigned");
            auto const size = std::distance (first, last);
            assert (size >= 0);

// FIXME: this should be a real check which is evaluated in a release build as well.
#ifndef NDEBUG
            auto const usize =
                static_cast<typename std::make_unsigned<decltype (size)>::type> (size);
            assert (usize >= std::numeric_limits<IntType>::min ());
            assert (usize <= std::numeric_limits<IntType>::max ());
#endif
            return static_cast<IntType> (size);
        }

        // size_bytes
        // ~~~~~~~~~~
        template <typename DataRange, typename IFixupRange, typename XFixupRange>
        std::size_t section::size_bytes (DataRange const & d, IFixupRange const & i,
                                         XFixupRange const & x) {
            auto const data_size = std::distance (d.first, d.second);
            auto const num_ifixups = std::distance (i.first, i.second);
            auto const num_xfixups = std::distance (x.first, x.second);
            assert (data_size >= 0 && num_ifixups >= 0 && num_xfixups >= 0);
            return size_bytes (static_cast<std::size_t> (data_size),
                               static_cast<std::size_t> (num_ifixups),
                               static_cast<std::size_t> (num_xfixups));
        }

        // num_ifixups
        // ~~~~~~~~~~~
        inline std::uint32_t section::num_ifixups () const noexcept {
            static_assert (sizeof (num_ifixups_) == 3, "num_ifixups is expected to be 3 bytes");
            return three_byte_integer::get (num_ifixups_);
        }

        // set_num_ifixups
        // ~~~~~~~~~~~~~~~
        template <typename Iterator>
        inline void section::set_num_ifixups (Iterator first, Iterator last, std::uint8_t * out) {
            constexpr auto out_bytes = std::size_t{3};
            static_assert (sizeof (num_ifixups_) == out_bytes,
                           "num_ifixups is expected to be 3 bytes");

            auto const size = std::distance (first, last);
            // FIXME: this should be a real runtime check even in a release build.
            assert (size >= 0 && size < (1U << (out_bytes * 8)));
            three_byte_integer::set (out, static_cast<std::uint32_t> (size));
        }


        struct section_content {
            section_content (section_kind kind_, std::uint8_t align_) noexcept
                    : kind{kind_}
                    , align{align_} {}
            section_content (section_content const &) = delete;
            section_content (section_content &&) = default;
            section_content & operator= (section_content const &) = delete;
            section_content & operator= (section_content &&) = default;

            template <typename Iterator>
            using range = std::pair<Iterator, Iterator>;

            template <typename Iterator>
            static inline auto make_range (Iterator begin, Iterator end) -> range<Iterator> {
                return {begin, end};
            }

            section_kind kind;
            std::uint8_t align;
            small_vector<std::uint8_t, 128> data;
            std::vector<internal_fixup> ifixups;
            std::vector<external_fixup> xfixups;

            auto make_sources () const
                -> section::sources<range<decltype (data)::const_iterator>,
                                    range<decltype (ifixups)::const_iterator>,
                                    range<decltype (xfixups)::const_iterator>> {

                return section::make_sources (
                    make_range (std::begin (data), std::end (data)),
                    make_range (std::begin (ifixups), std::end (ifixups)),
                    make_range (std::begin (xfixups), std::end (xfixups)));
            }
        };

        namespace details {

            /// An iterator adaptor which produces a value_type of 'section_kind const' from
            /// values deferenced from the supplied underlying iterator.
            template <typename Iterator>
            class content_type_iterator {
            public:
                using value_type = section_kind const;
                using difference_type = typename std::iterator_traits<Iterator>::difference_type;
                using pointer = value_type *;
                using reference = value_type &;
                using iterator_category = std::input_iterator_tag;

                content_type_iterator ()
                        : it_{} {}
                explicit content_type_iterator (Iterator it)
                        : it_{it} {}
                content_type_iterator (content_type_iterator const & rhs)
                        : it_{rhs.it_} {}
                content_type_iterator & operator= (content_type_iterator const & rhs) {
                    it_ = rhs.it_;
                    return *this;
                }
                bool operator== (content_type_iterator const & rhs) const { return it_ == rhs.it_; }
                bool operator!= (content_type_iterator const & rhs) const {
                    return !(operator== (rhs));
                }
                content_type_iterator & operator++ () {
                    ++it_;
                    return *this;
                }
                content_type_iterator operator++ (int) {
                    auto const old = *this;
                    it_++;
                    return old;
                }

                reference operator* () const { return (*it_).kind (); }
                pointer operator-> () const { return &(*it_).kind (); }
                reference operator[] (difference_type n) const { return it_[n].kind (); }

            private:
                Iterator it_;
            };

            template <typename Iterator>
            inline content_type_iterator<Iterator> make_content_type_iterator (Iterator it) {
                return content_type_iterator<Iterator> (it);
            }

            /// An iterator adaptor which produces a value_type which dereferences the
            /// value_type of the wrapped iterator.
            template <typename Iterator>
            class fragment_content_iterator {
            public:
                using value_type = typename std::pointer_traits<
                    typename std::iterator_traits<Iterator>::value_type>::element_type;
                using difference_type = typename std::iterator_traits<Iterator>::difference_type;
                using pointer = value_type *;
                using reference = value_type &;
                using iterator_category = std::input_iterator_tag;

                fragment_content_iterator ()
                        : it_{} {}
                explicit fragment_content_iterator (Iterator it)
                        : it_{it} {}
                fragment_content_iterator (fragment_content_iterator const & rhs)
                        : it_{rhs.it_} {}
                fragment_content_iterator & operator= (fragment_content_iterator const & rhs) {
                    it_ = rhs.it_;
                    return *this;
                }
                bool operator== (fragment_content_iterator const & rhs) const {
                    return it_ == rhs.it_;
                }
                bool operator!= (fragment_content_iterator const & rhs) const {
                    return !(operator== (rhs));
                }
                fragment_content_iterator & operator++ () {
                    ++it_;
                    return *this;
                }
                fragment_content_iterator operator++ (int) {
                    fragment_content_iterator old{*this};
                    it_++;
                    return old;
                }

                reference operator* () const { return **it_; }
                pointer operator-> () const { return &(**it_); }
                reference operator[] (difference_type n) const { return *(it_[n]); }

            private:
                Iterator it_;
            };

            template <typename Iterator>
            inline fragment_content_iterator<Iterator>
            make_fragment_content_iterator (Iterator it) {
                return fragment_content_iterator<Iterator> (it);
            }

        } // end namespace details


        //*     _                       _         _       *
        //*  __| |___ _ __  ___ _ _  __| |___ _ _| |_ ___ *
        //* / _` / -_) '_ \/ -_) ' \/ _` / -_) ' \  _(_-< *
        //* \__,_\___| .__/\___|_||_\__,_\___|_||_\__/__/ *
        //*          |_|								  *
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


		//*                  _   _               _ _               _      _                *
        //*  __ _ _ ___ __ _| |_(_)___ _ _    __| (_)____ __  __ _| |_ __| |_  ___ _ _ ___ *
        //* / _| '_/ -_) _` |  _| / _ \ ' \  / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_(_-< *
        //* \__|_| \___\__,_|\__|_\___/_||_| \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_| /__/ *
        //*                                            |_|                                 *
        /// A section creation dispatcher is used to instantiate and construct each of a fragment's
        /// sections in pstore memory. Objects in the pstore need to be portable across compilers
        /// and host ABIs so they must be "standard layout" which basically means that they can't
        /// have virtual member functions. These classes are used to add dynamic dispatch to those
        /// types.
        ///
        /// \note In addition to the "section creation" dispatcher, there is a second dispatcher
        /// hierarchy used to provide dynamic behavior for existing section instances.
        class section_creation_dispatcher {
        public:
            explicit section_creation_dispatcher (section_kind kind) noexcept
                    : kind_{kind} {}
            virtual ~section_creation_dispatcher () noexcept = default;
            section_creation_dispatcher (section_creation_dispatcher const &) = delete;
            section_creation_dispatcher & operator= (section_creation_dispatcher const &) = delete;

            section_kind const & kind () const noexcept { return kind_; }

            /// \param a  The value to be aligned.
            /// \returns The value closest to but greater than or equal to \p a which is correctly
            /// aligned for an instance of the type used for an instance of this section kind.
            template <typename IntType, typename = std::enable_if<std::is_unsigned<IntType>::value>>
            std::size_t aligned (IntType a) const {
                static_assert (sizeof (std::uintptr_t) >= sizeof (IntType),
                               "sizeof uintptr_t must be at least sizeof IntType");
                return static_cast<std::size_t> (
                    this->aligned_impl (static_cast<std::uintptr_t> (a)));
            }
            /// \param a  The value to be aligned.
            /// \returns The value closest to but greater than or equal to \p a which is correctly
            /// aligned for an instance of the type used for an instance of this section kind.
            std::uint8_t * aligned (std::uint8_t * a) const {
                return reinterpret_cast<std::uint8_t *> (
                    this->aligned_impl (reinterpret_cast<std::uintptr_t> (a)));
            }

            /// Returns the number of bytes of storage that are required for an instance of the
            /// section data.
            virtual std::size_t size_bytes () const = 0;

            /// Copies the section instance data to the memory starting at \p out. On entry, \p out
            /// is aligned according to the result of the aligned() member function. \param out  The
            /// address to which the instance data will be written. \returns The address past the
            /// end of instance data where the next section's data can be writen.
            virtual std::uint8_t * write (std::uint8_t * out) const = 0;

        private:
            /// \param v  The value to be aligned.
            /// \returns The value closest to but greater than or equal to \p v which is correctly
            /// aligned for an instance of the type used for an instance of this section kind.
            virtual std::uintptr_t aligned_impl (std::uintptr_t v) const = 0;

            section_kind const kind_;
        };

        class generic_section_creation_dispatcher final : public section_creation_dispatcher {
        public:
            generic_section_creation_dispatcher (section_kind const kind,
                                                 section_content const * sec)
                    : section_creation_dispatcher (kind)
                    , section_ (sec) {}

            generic_section_creation_dispatcher (generic_section_creation_dispatcher const &) =
                delete;
            generic_section_creation_dispatcher &
            operator= (generic_section_creation_dispatcher const &) = delete;

            std::size_t size_bytes () const final;

            // Write the section data to the memory which the pointer 'out' pointed to.
            std::uint8_t * write (std::uint8_t * out) const final;

        private:
            std::uintptr_t aligned_impl (std::uintptr_t in) const final;
            section_content const * const section_;
        };

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


        /// Maps from the section kind enumeration to the type that is used to represent a section
        /// of that kind.
        template <section_kind T>
        struct enum_to_section {};
#    define PSTORE_ENUM_TO_SECTION(t, stored_type)                                                 \
        template <>                                                                                \
        struct enum_to_section<t> {                                                                \
            using type = stored_type;                                                              \
        };
        PSTORE_ENUM_TO_SECTION (section_kind::bss, section)
        PSTORE_ENUM_TO_SECTION (section_kind::data, section)
        PSTORE_ENUM_TO_SECTION (section_kind::mergeable_1_byte_c_string, section)
        PSTORE_ENUM_TO_SECTION (section_kind::mergeable_2_byte_c_string, section)
        PSTORE_ENUM_TO_SECTION (section_kind::mergeable_4_byte_c_string, section)
        PSTORE_ENUM_TO_SECTION (section_kind::mergeable_const_16, section)
        PSTORE_ENUM_TO_SECTION (section_kind::mergeable_const_32, section)
        PSTORE_ENUM_TO_SECTION (section_kind::mergeable_const_4, section)
        PSTORE_ENUM_TO_SECTION (section_kind::mergeable_const_8, section)
        PSTORE_ENUM_TO_SECTION (section_kind::read_only, section)
        PSTORE_ENUM_TO_SECTION (section_kind::rel_ro, section)
        PSTORE_ENUM_TO_SECTION (section_kind::text, section)
        PSTORE_ENUM_TO_SECTION (section_kind::thread_bss, section)
        PSTORE_ENUM_TO_SECTION (section_kind::thread_data, section)
        PSTORE_ENUM_TO_SECTION (section_kind::dependent, dependents)
#    undef PSTORE_ENUM_TO_SECTION

        namespace details {

            /// Provides a member typedef inherit_const::type, which is defined as \p R const if
            /// \p T is a const type and \p R if \p T is non-const.
            ///
            /// \tparam T  A type whose constness will determine the constness of
            ///   inherit_const::type.
            /// \tparam R  The result type with added const if \p T is const.
            template <typename T, typename R>
            struct inherit_const {
                /// If \p T is const, \p R const otherwise \p R.
                using type = typename std::conditional<std::is_const<T>::value, R const, R>::type;
            };

        } // end namespace details

        //*   __                             _    *
        //*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_  *
        //* |  _| '_/ _` / _` | '  \/ -_) ' \  _| *
        //* |_| |_| \__,_\__, |_|_|_\___|_||_\__| *
        //*              |___/                    *
        class fragment {
        public:
            using member_array = sparse_array<std::uint64_t>;

            /// Prepares an instance of a fragment with the collection of sections defined by the
            /// iterator range [first, last).
            ///
            /// \tparam Transaction  The type of the database transaction.
            /// \tparam Iterator  An iterator which will yield values of type
            ///   pstore::repo::section_creation_dispatcher. The values must be sorted in
            ///   pstore::repo::section_content::kind order.
            /// \param transaction  The transaction to which the fragment should be appended.
            /// \param first  The beginning of the range of
            ///   pstore::repo::section_creation_dispatcher values.
            /// \param last  The end of the range of pstore::repo::section_creation_dispatcher
            ///   values.
            template <typename Transaction, typename Iterator>
            static pstore::extent<fragment> alloc (Transaction & transaction, Iterator first,
                                                   Iterator last);

            /// Provides a pointer to an individual fragment instance given a database and a extent
            /// describing its address and size.
            ///
            /// \param db  The database from which the fragment is to be read.
            /// \param location  The address and size of the fragment data.
            /// \returns  A pointer to the fragment instance.
            static std::shared_ptr<fragment const> load (database const & db,
                                                         extent<fragment> const & location);

            /// Provides a pointer to an individual fragment instance given a transaction and an
            /// extent describing its address and size.
            ///
            /// \tparam LockType  The transaction lock type.
            /// \param transaction  The transaction from which the fragment is to be read.
            /// \param location  The address and size of the fragment data.
            /// \returns  A pointer to the fragment instance.
            template <typename LockType>
            static std::shared_ptr<fragment> load (transaction<LockType> & transaction,
                                                   extent<fragment> const & location);

            /// Returns true if the fragment contains a section of the kind given by \p kind, false
            /// otherwise. \param kind  The section kind to check. \returns Returns true if the
            /// fragment contains a section of the kind given by \p kind, false otherwise.
            bool has_section (section_kind kind) const noexcept {
                return arr_.has_index (static_cast<member_array::bitmap_type> (kind));
            }

            /// \name Section Access
            ///@{

            /// Returns a const reference to the section data given the section kind. The section
            /// must exist in the fragment.
            template <section_kind Key>
            auto at () const noexcept -> typename enum_to_section<Key>::type const & {
                return at_impl<Key> (*this);
            }
            /// Returns a reference to the section data given the section kind. The section must
            /// exist in the fragment.
            template <section_kind Key>
            auto at () noexcept -> typename enum_to_section<Key>::type & {
                return at_impl<Key> (*this);
            }

            /// Returns a pointer to the section data given the section kind or nullptr if the
            /// section is not present.
            template <section_kind Key>
            auto atp () const noexcept -> typename enum_to_section<Key>::type const * {
                return atp_impl<Key> (*this);
            }
            /// Returns a pointer to the section data given the section kind or nullptr if the
            /// section is not present.
            template <section_kind Key>
            auto atp () noexcept -> typename enum_to_section<Key>::type * {
                return atp_impl<Key> (*this);
            }
            ///@}

            /// Return the number of sections in the fragment.
            std::size_t size () const noexcept { return arr_.size (); }

            /// Returns the array of section offsets.
            member_array const & members () const noexcept { return arr_; }

            /// An iterator which makes the process of iterating over the sections within
            /// a loaded fragment quite straightforward.
            class const_iterator {
                using wrapped_iterator = member_array::indices::const_iterator;
            public:
                using iterator_category = std::forward_iterator_tag;
                using value_type = section_kind;
                using difference_type = wrapped_iterator::difference_type;
                using pointer = value_type const *;
                using reference = value_type const &;

                const_iterator (section_kind kind, wrapped_iterator it) noexcept;
                explicit const_iterator (wrapped_iterator it) noexcept;
                bool operator== (const_iterator const & rhs) const noexcept {
                    return it_ == rhs.it_;
                }
                bool operator!= (const_iterator const & rhs) const noexcept {
                    return !operator== (rhs);
                }

                reference operator* () const noexcept { return section_kind_; }
                pointer operator-> () const noexcept { return &section_kind_; }
                const_iterator & operator++ () noexcept;
                const_iterator operator++ (int) noexcept;

            private:
                section_kind section_kind_;
                wrapped_iterator it_;
            };

            const_iterator begin () const noexcept {
                return const_iterator{arr_.get_indices ().begin ()};
            }
            const_iterator end () const noexcept {
                return const_iterator{section_kind::last, arr_.get_indices ().end ()};
            }

            /// Returns the number of bytes of storage that are required for a fragment containing
            /// the sections defined by [first, last).
            ///
            /// \tparam Iterator  An iterator which will yield values of type
            ///   pstore::repo::section_content. The values must be sorted in
            ///   pstore::repo::section_content::kind order.
            /// \param first  The beginning of the range of pstore::repo::section_content values.
            /// \param last  The end of the range of pstore::repo::section_content values.
            template <typename Iterator>
            static std::size_t size_bytes (Iterator first, Iterator last);

            /// Returns the number of bytes of storage that are accupied by this fragment.
            std::size_t size_bytes () const;

        private:
            template <typename IteratorIdx>
            fragment (IteratorIdx first_index, IteratorIdx last_index)
                    : arr_ (first_index, last_index) {
                static_assert (
                    std::numeric_limits<member_array::bitmap_type>::radix == 2,
                    "expect numeric radix to be 2 (so that 'digits' is the number of bits)");
                using utype = std::underlying_type<section_kind>::type;
                static_assert (static_cast<utype> (section_kind::last) <=
                                   std::numeric_limits<member_array::bitmap_type>::digits,
                               "section_kind does not fit in the member spare array");
            }

            /// Returns pointer to an individual fragment instance given a function which can yield
            /// it given the object's extent.
            template <typename ReturnType, typename GetOp>
            static ReturnType load_impl (extent<fragment> const & location, GetOp get);

            template <typename Iterator>
            static void check_range_is_sorted (Iterator first, Iterator last);

            ///@{
            /// Yields a reference to section data found at a known offset within the fragment
            /// payload. \param offset The number of bytes from the start of the fragment at which
            /// the data lies.

            template <typename InstanceType>
            InstanceType const & offset_to_instance (std::uint64_t offset) const noexcept {
                return offset_to_instance_impl<InstanceType const> (*this, offset);
            }
            template <typename InstanceType>
            InstanceType & offset_to_instance (std::uint64_t offset) noexcept {
                return offset_to_instance_impl<InstanceType> (*this, offset);
            }
            ///@}


            /// The implementation of offset_to_instance<>() (used by the const and non-const
            /// flavors).
            template <typename InstanceType, typename Fragment>
            static InstanceType & offset_to_instance_impl (Fragment & f,
                                                           std::uint64_t offset) noexcept;

            /// The implementation of at<>() (used by the const and non-const flavors).
            ///
            /// \tparam Key The section_kind to be accessed.
            /// \tparam Fragment Const- or non-const fragment.
            /// \tparam ResultType The function result type, normally derived automatically from the
            ///   Key and Fragment template arguments.
            /// \param f  The fragment to be accessed.
            /// \returns A constant reference to the instance contained in the Key section if the
            /// input fragment
            ///   type is const; non-const otherwise.
            template <section_kind Key, typename Fragment,
                      typename ResultType = typename details::inherit_const<
                          Fragment, typename enum_to_section<Key>::type>::type>
            static ResultType & at_impl (Fragment & f) {
                assert (f.has_section (Key));
                return f.template offset_to_instance<ResultType> (
                    f.arr_[static_cast<std::size_t> (Key)]);
            }

            /// The implementation of atp<>() (used by the const and non-const flavors).
            ///
            /// \tparam Key The section_kind to be accessed.
            /// \tparam Fragment Const- or non-const fragment.
            /// \tparam ResultType The function result type, normally derived automatically from the
            ///   Key and Fragment template arguments.
            /// \param f  The fragment to be accessed.
            /// \returns A constant pointer to the instance contained in the Key section if the
            /// input fragment
            ///   type is const; non-const otherwise.
            template <section_kind Key, typename Fragment,
                      typename ResultType = typename details::inherit_const<
                          Fragment, typename enum_to_section<Key>::type>::type>
            static ResultType * atp_impl (Fragment & f) {
                return f.has_section (Key) ? &f.template at<Key> () : nullptr;
            }

            /// Constructs a fragment into the uninitialized memory referred to by ptr and copies
            /// the section contents [first,last) into it.
            ///
            /// \tparam Iterator  An iterator which will yield values of type
            ///   pstore::repo::section_content. The values must be sorted in
            ///   pstore::repo::section_content::kind order.
            /// \param ptr  A pointer to an uninitialized
            ///   block of memory of at least the size returned by size_bytes(first, last).
            /// \param first  The beginning of the range of pstore::repo::section_content values.
            /// \param last  The end of the range of pstore::repo::section_content values.
            template <typename Iterator>
            static void populate (void * ptr, Iterator first, Iterator last);

            /// A sparse array of offsets to each of the contained sections.
            member_array arr_;
        };

        // operator<<
        // ~~~~~~~~~~
        template <typename OStream>
        OStream & operator<< (OStream & os, section_kind kind) {
            char const * name = "*unknown*";
            switch (kind) {
            case section_kind::text: name = "text"; break;
            case section_kind::bss: name = "bss"; break;
            case section_kind::data: name = "data"; break;
            case section_kind::rel_ro: name = "rel_ro"; break;
            case section_kind::mergeable_1_byte_c_string: name = "mergeable_1_byte_c_string"; break;
            case section_kind::mergeable_2_byte_c_string: name = "mergeable_2_byte_c_string"; break;
            case section_kind::mergeable_4_byte_c_string: name = "mergeable_4_byte_c_string"; break;
            case section_kind::mergeable_const_4: name = "mergeable_const_4"; break;
            case section_kind::mergeable_const_8: name = "mergeable_const_8"; break;
            case section_kind::mergeable_const_16: name = "mergeable_const_16"; break;
            case section_kind::mergeable_const_32: name = "mergeable_const_32"; break;
            case section_kind::read_only: name = "read_only"; break;
            case section_kind::thread_bss: name = "thread_bss"; break;
            case section_kind::thread_data: name = "thread_data"; break;
            case section_kind::dependent: name = "dependent"; break;
            case section_kind::last: assert (false); break;
            }
            return os << name;
        }

        // populate [private, static]
        // ~~~~~~~~
        template <typename Iterator>
        void fragment::populate (void * ptr, Iterator first, Iterator last) {
            // Construct the basic fragment structure into this memory.
            auto fragment_ptr = new (ptr) fragment (details::make_content_type_iterator (first),
                                                    details::make_content_type_iterator (last));
            // Point past the end of the sparse array.
            auto out =
                reinterpret_cast<std::uint8_t *> (fragment_ptr) + fragment_ptr->arr_.size_bytes ();

            // Copy the contents of each of the segments to the fragment.
            auto op = [&out, fragment_ptr](section_creation_dispatcher const & c) {
                auto const index = static_cast<unsigned> (c.kind ());
                out = reinterpret_cast<std::uint8_t *> (c.aligned (out));
                fragment_ptr->arr_[index] = reinterpret_cast<std::uintptr_t> (out) -
                                            reinterpret_cast<std::uintptr_t> (fragment_ptr);
                out = c.write (out);
            };
            std::for_each (first, last, op);
#ifndef NDEBUG
            {
                auto const size = fragment::size_bytes (first, last);
                assert (out >= ptr && static_cast<std::size_t> (
                                          out - reinterpret_cast<std::uint8_t *> (ptr)) == size);
                assert (size == fragment_ptr->size_bytes ());
            }
#endif
        }

        // alloc
        // ~~~~~
        template <typename Transaction, typename Iterator>
        auto fragment::alloc (Transaction & transaction, Iterator first, Iterator last)
            -> pstore::extent<fragment> {
            fragment::check_range_is_sorted (first, last);
            // Compute the number of bytes of storage that we'll need for this fragment.
            auto const size = fragment::size_bytes (first, last);

            // Allocate storage for the fragment including its three arrays.
            // We can't use the version of alloc_rw() which returns typed_address<> since we need
            // an explicit number of bytes allocated for the fragment.
            std::pair<std::shared_ptr<void>, pstore::address> storage =
                transaction.alloc_rw (size, alignof (fragment));
            fragment::populate (storage.first.get (), first, last);
            return {typed_address<fragment> (storage.second), size};
        }

        // load_impl
        // ~~~~~~~~~
        template <typename ReturnType, typename GetOp>
        auto fragment::load_impl (extent<fragment> const & location, GetOp get) -> ReturnType {
            if (location.size >= sizeof (fragment)) {
                ReturnType f = get (location);
                auto const indices = f->members ().get_indices ();

                auto is_valid_index = [](unsigned k) {
                    return k < static_cast<unsigned> (section_kind::last);
                };
                if (!indices.empty () && is_valid_index (indices.back ()) &&
                    f->size_bytes () == location.size) {
                    return f;
                }
            }
            raise_error_code (std::make_error_code (error_code::bad_fragment_record));
        }

        // load
        // ~~~~~
        template <typename LockType>
        auto fragment::load (transaction<LockType> & transaction,
                             pstore::extent<fragment> const & location)
            -> std::shared_ptr<fragment> {
            return load_impl<std::shared_ptr<fragment>> (
                location,
                [&transaction](extent<fragment> const & x) { return transaction.getrw (x); });
        }


        // offset_to_instance
        // ~~~~~~~~~~~~~~~~~~
        template <typename InstanceType, typename Fragment>
        InstanceType & fragment::offset_to_instance_impl (Fragment & f,
                                                          std::uint64_t offset) noexcept {
            // This is the implementation used by both const and non-const flavors of
            // offset_to_instance().
            auto const ptr =
                reinterpret_cast<typename details::inherit_const<Fragment, std::uint8_t>::type *> (
                    &f) +
                offset;
            assert (reinterpret_cast<std::uintptr_t> (ptr) % alignof (InstanceType) == 0);
            return *reinterpret_cast<InstanceType *> (ptr);
        }

        // check_range_is_sorted
        // ~~~~~~~~~~~~~~~~~~~~~
        template <typename Iterator>
        void fragment::check_range_is_sorted (Iterator first, Iterator last) {
            (void) first;
            (void) last;
            using value_type = typename std::iterator_traits<Iterator>::value_type;
            assert (std::is_sorted (first, last, [](value_type const & a, value_type const & b) {
                section_kind const akind = a.kind ();
                section_kind const bkind = b.kind ();
                using utype = std::underlying_type<section_kind>::type;
                return static_cast<utype> (akind) < static_cast<utype> (bkind);
            }));
        }

        // size_bytes
        // ~~~~~~~~~~
        template <typename Iterator>
        std::size_t fragment::size_bytes (Iterator first, Iterator last) {
            fragment::check_range_is_sorted (first, last);

            auto const num_contents = std::distance (first, last);
            assert (num_contents >= 0);
            auto const unum_contents =
                static_cast<typename std::make_unsigned<decltype (num_contents)>::type> (
                    num_contents);

            // Space needed by the section offset array.
            std::size_t size_bytes = decltype (fragment::arr_)::size_bytes (unum_contents);
            // Now the storage for each of the contents
            std::for_each (first, last, [&size_bytes](section_creation_dispatcher const & c) {
                size_bytes = c.aligned (size_bytes);
                size_bytes += c.size_bytes ();
            });
            return size_bytes;
        }


        // fragment::iterator
        // ~~~~~~~~~~~~~~~~~~
        inline fragment::const_iterator::const_iterator (section_kind kind,
                                                         wrapped_iterator it) noexcept
                : section_kind_{kind}
                , it_{it} {}

        inline fragment::const_iterator::const_iterator (wrapped_iterator it) noexcept
                : const_iterator (static_cast<section_kind> (*it), it) {
            assert (*it < static_cast<std::uint64_t> (section_kind::last));
        }

        inline fragment::const_iterator & fragment::const_iterator::operator++ () noexcept {
            ++it_;
            section_kind_ = static_cast<section_kind> (*it_);
            return *this;
        }
        inline fragment::const_iterator fragment::const_iterator::operator++ (int) noexcept {
            auto const prev = *this;
            ++*this;
            return prev;
        }

        /// Returns the alignment of the given section type in the given fragment.
        unsigned section_align (fragment const & fragment, section_kind kind);

        /// Returns the number of bytes of storage that are accupied by the section with the
        /// given type in the given fragment.
        std::size_t section_size (fragment const & fragment, section_kind kind);

        /// Returns the ifixups of the given section type in the given fragment.
        section::container<internal_fixup> section_ifixups (fragment const & fragment,
                                                            section_kind kind);

        /// Returns the xfixups of the given section type in the given fragment.
        section::container<external_fixup> section_xfixups (fragment const & fragment,
                                                            section_kind kind);

        /// Returns the section content of the given section type in the given fragment.
        section::container<std::uint8_t> section_value (fragment const & fragment,
                                                        section_kind kind);
    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_FRAGMENT_HPP
