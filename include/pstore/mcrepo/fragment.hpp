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
// RepoObjectWriter::writeRepoSectionData(). It's missing at least the debugging, and EH-related
// sections and probably others...
#define PSTORE_REPO_SECTION_TYPES                                                                  \
    X (text)                                                                                       \
    X (bss)                                                                                        \
    X (data)                                                                                       \
    X (rel_ro)                                                                                     \
    X (mergeable_1_byte_c_string)                                                                  \
    X (mergeable_2_byte_c_string)                                                                  \
    X (mergeable_4_byte_c_string)                                                                  \
    X (mergeable_const_4)                                                                          \
    X (mergeable_const_8)                                                                          \
    X (mergeable_const_16)                                                                         \
    X (mergeable_const_32)                                                                         \
    X (read_only)                                                                                  \
    X (thread_bss)                                                                                 \
    X (thread_data)

#define PSTORE_REPO_METADATA_TYPES X (dependent)

#define X(a) a,
        /// \brief The collection of section types known by the repository.
        enum class section_type : std::uint8_t { PSTORE_REPO_SECTION_TYPES last };

        enum class fragment_type : std::uint8_t {
            PSTORE_REPO_SECTION_TYPES PSTORE_REPO_METADATA_TYPES last
        };
#undef X

        inline bool is_section_type (fragment_type const t) noexcept {
            return t < static_cast<repo::fragment_type> (repo::section_type::last);
        }

        using relocation_type = std::uint8_t;

        //*  _     _                     _    __ _                *
        //* (_)_ _| |_ ___ _ _ _ _  __ _| |  / _(_)_ ___  _ _ __  *
        //* | | ' \  _/ -_) '_| ' \/ _` | | |  _| \ \ / || | '_ \ *
        //* |_|_||_\__\___|_| |_||_\__,_|_| |_| |_/_\_\\_,_| .__/ *
        //*                                                |_|    *
        struct internal_fixup {
            internal_fixup (section_type section_, relocation_type type_, std::uint64_t offset_,
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

            section_type section;
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

        //*   __                             _        _      _         *
        //*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_   __| |__ _| |_ __ _  *
        //* |  _| '_/ _` / _` | '  \/ -_) ' \  _| / _` / _` |  _/ _` | *
        //* |_| |_| \__,_\__, |_|_|_\___|_||_\__| \__,_\__,_|\__\__,_| *
        //*              |___/                                         *
        //*                                                            *
        struct fragment_data {
            explicit fragment_data (fragment_type ft)
                    : type (ft) {}

            virtual ~fragment_data () = default;

            std::size_t aligned_size_t (std::size_t a) const;
            std::uint8_t * aligned_ptr (std::uint8_t * in) const;

            virtual std::size_t size_bytes () const = 0;
            virtual std::uint8_t * write (std::uint8_t * out) const = 0;
            fragment_type const type;

        private:
            virtual std::uintptr_t aligned (std::uintptr_t) const = 0;
        };

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
            section_content (section_type st, std::uint8_t align_)
                    : type{st}
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

            section_type type;
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

            /// An iterator adaptor which produces a value_type of 'fragment_data const' from
            /// values deferenced from the supplied underlying iterator.
            template <typename Iterator>
            class content_type_iterator {
            public:
                using value_type = fragment_type const;
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

                reference operator* () const { return (*it_).type; }
                pointer operator-> () const { return &(*it_).type; }
                reference operator[] (difference_type n) const { return it_[n].type; }

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

        //*             _   _               _      _         *
        //*  ___ ___ __| |_(_)___ _ _    __| |__ _| |_ __ _  *
        //* (_-</ -_) _|  _| / _ \ ' \  / _` / _` |  _/ _` | *
        //* /__/\___\__|\__|_\___/_||_| \__,_\__,_|\__\__,_| *
        //*                          						 *
        class section_data final : public fragment_data {
        public:
            section_data (fragment_type const ft, section_content const * sec)
                    : fragment_data (ft)
                    , section_ (sec) {}

            section_data (section_data const &) = delete;
            section_data & operator= (section_data const &) = delete;


            std::size_t size_bytes () const final;

            // Write the section data to the memory which the pointer 'out' pointed to.
            std::uint8_t * write (std::uint8_t * out) const final;

        private:
            std::uintptr_t aligned (std::uintptr_t in) const final;
            section_content const * const section_;
        };



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

        //*     _                       _         _           _      _         *
        //*  __| |___ _ __  ___ _ _  __| |___ _ _| |_ ___  __| |__ _| |_ __ _  *
        //* / _` / -_) '_ \/ -_) ' \/ _` / -_) ' \  _(_-< / _` / _` |  _/ _` | *
        //* \__,_\___| .__/\___|_||_\__,_\___|_||_\__/__/ \__,_\__,_|\__\__,_| *
        //*          |_|                                                       *
        class dependents_data final : public fragment_data {
        public:
            using const_iterator = typed_address<ticket_member> const *;

            dependents_data (const_iterator begin, const_iterator end)
                    : fragment_data (fragment_type::dependent)
                    , begin_ (begin)
                    , end_ (end) {
                assert (end >= begin);
            }

            std::size_t size_bytes () const final;

            std::uint8_t * write (std::uint8_t * out) const final;

            /// Returns a const_iterator for the beginning of the ticket_member address range.
            const_iterator begin () const { return begin_; }
            /// Returns a const_iterator for the end of ticket_member address range.
            const_iterator end () const { return end_; }

        private:
            std::uintptr_t aligned (std::uintptr_t in) const final {
                return pstore::aligned<dependents> (in);
            }

            const_iterator begin_;
            const_iterator end_;
        };

        template <fragment_type T>
        struct enum_to_section {};
#    define PSTORE_ENUM_TO_SECTION(t, stored_type)                                                 \
        template <>                                                                                \
        struct enum_to_section<t> {                                                                \
            using type = stored_type;                                                              \
        };
        PSTORE_ENUM_TO_SECTION (fragment_type::bss, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::data, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::mergeable_1_byte_c_string, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::mergeable_2_byte_c_string, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::mergeable_4_byte_c_string, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::mergeable_const_16, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::mergeable_const_32, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::mergeable_const_4, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::mergeable_const_8, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::read_only, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::rel_ro, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::text, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::thread_bss, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::thread_data, section)
        PSTORE_ENUM_TO_SECTION (fragment_type::dependent, dependents)
#    undef PSTORE_ENUM_TO_SECTION

        //*   __                             _    *
        //*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_  *
        //* |  _| '_/ _` / _` | '  \/ -_) ' \  _| *
        //* |_| |_| \__,_\__, |_|_|_\___|_||_\__| *
        //*              |___/                    *
        class fragment {
        public:
            /// \tparam Transaction  The type of the database transaction.
            /// \tparam Iterator  An iterator which will yield values of type `fragment_data`. The
            /// values must be sorted in section_content::type order.
            /// \param transaction  The transaction to which the fragment should be appended.
            /// \param first  The beginning of the range of `fragment_data` values.
            /// \param last  The end of the range of `fragment_data` values.
            template <typename Transaction, typename Iterator>
            static pstore::extent<fragment> alloc (Transaction & transaction, Iterator first,
                                                   Iterator last);

            /// Provides a pointer to an individual fragment instance given a database and a extent
            /// describing its address and size.
            ///
            /// \param db  The database from which the fragment is to be read.
            /// \param location  The address and size of the fragment data.
            /// \returns  A pointer to the fragment instance.
            static std::shared_ptr<fragment const> load (pstore::database const & db,
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

            using member_array = sparse_array<std::uint64_t>;

            /// Returns a reference to the section type from section enum type in the fragment.
            /// The section must exist in the fragment.
            template <fragment_type Key>
            auto at () const noexcept -> typename enum_to_section<Key>::type const &;

            /// Returns true if this fragment contains a section of the given type.
            bool has_fragment (fragment_type type) const noexcept;

            /// Return a pointer to a dependents fragment instance in the fragment.
            class dependents const * dependents () const noexcept;
            class dependents * dependents () noexcept;

            /// Return a number of sections in the fragment.
            std::size_t num_sections () const noexcept;

            /// Return a number of instances (include the sections and dependents) in the fragment.
            std::size_t size () const noexcept;

            /// Returns the array of section offsets.
            member_array const & members () const noexcept { return arr_; }

            /// An InputIterator which makes the process of iterating over the sections within
            /// a loaded fragment quite straightforward.
            class const_iterator {
                using wrapped_iterator = member_array::indices::const_iterator;

            public:
                using iterator_category = std::input_iterator_tag;
                using value_type = fragment_type;
                using difference_type = wrapped_iterator::difference_type;
                using pointer = value_type const *;
                using reference = value_type const &;

                explicit const_iterator (fragment_type type, wrapped_iterator it) noexcept;
                explicit const_iterator (wrapped_iterator it) noexcept;
                bool operator== (const_iterator const & rhs) const noexcept {
                    return it_ == rhs.it_;
                }
                bool operator!= (const_iterator const & rhs) const noexcept {
                    return !operator== (rhs);
                }

                reference operator* () const noexcept;
                pointer operator-> () const noexcept;

                const_iterator & operator++ () noexcept;
                const_iterator operator++ (int) noexcept;

            private:
                fragment_type fragment_;
                wrapped_iterator it_;
            };

            const_iterator begin () const noexcept {
                return const_iterator{arr_.get_indices ().begin ()};
            }
            const_iterator end () const noexcept {
                return const_iterator{fragment_type::last, arr_.get_indices ().end ()};
            }

            /// Returns the number of bytes of storage that are required for a fragment containing
            /// the sections defined by [first, last).
            ///
            /// \tparam Iterator  An iterator which will yield values of type `section_content`. The
            /// values must be sorted in section_content::type order.
            /// \param first  The beginning of the range of `section_content` values.
            /// \param last  The end of the range of `section_content` values.
            template <typename Iterator>
            static std::size_t size_bytes (Iterator first, Iterator last);

            /// Returns the number of bytes of storage that are accupied by this fragment.
            std::size_t size_bytes () const;

        private:
            template <typename IteratorIdx>
            fragment (IteratorIdx first_index, IteratorIdx last_index)
                    : arr_ (first_index, last_index) {}

            /// Returns pointer to an individual fragment instance given a function which can yield
            /// it given the object's extent.
            template <typename ReturnType, typename GetOp>
            static ReturnType load_impl (extent<fragment> const & location, GetOp get);

            template <typename Iterator>
            static void check_range_is_sorted (Iterator first, Iterator last);

            template <typename InstanceType>
            InstanceType const & offset_to_instance (std::uint64_t offset) const noexcept;


            /// Constructs a fragment into the uninitialized memory referred to by ptr and copy the
            /// section contents [first,last) into it.
            ///
            /// \tparam Iterator  An iterator which will yield values of type `section_content`. The
            /// values must be sorted in section_content::type order.
            /// \param ptr  A pointer to an uninitialized block of memory of at least the size
            /// returned by size_bytes(first, last).
            /// \param first  The beginning of the range of `section_content` values.
            /// \param last  The end of the range of `section_content` values.
            template <typename Iterator>
            static void populate (void * ptr, Iterator first, Iterator last);

            /// A sparse array of offsets to each of the contained sections.
            member_array arr_;
        };

        // operator<<
        // ~~~~~~~~~~
        template <typename OStream>
        OStream & operator<< (OStream & os, section_type st) {
#define X(a)                                                                                       \
    case (section_type::a): name = #a; break;
            char const * name = "*unknown*";
            switch (st) {
                PSTORE_REPO_SECTION_TYPES
            case section_type::last: break;
            }
            return os << name;
#undef X
        }

        // populate
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
            std::for_each (first, last, [&out, fragment_ptr](fragment_data const & c) {
                out = reinterpret_cast<std::uint8_t *> (c.aligned_ptr (out));
                fragment_ptr->arr_[static_cast<unsigned> (c.type)] =
                    reinterpret_cast<std::uintptr_t> (out) -
                    reinterpret_cast<std::uintptr_t> (fragment_ptr);
                out = c.write (out);
            });
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
                auto ftype_indices = f->members ().get_indices ();
                if (!ftype_indices.empty () &&
                    static_cast<fragment_type> (ftype_indices.back ()) < fragment_type::last &&
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
        template <typename InstanceType>
        InstanceType const & fragment::offset_to_instance (std::uint64_t offset) const noexcept {
            auto const ptr = reinterpret_cast<std::uint8_t const *> (this) + offset;
            assert (reinterpret_cast<std::uintptr_t> (ptr) % alignof (InstanceType) == 0);
            return *reinterpret_cast<InstanceType const *> (ptr);
        }

        // at
        // ~~
        template <fragment_type Key>
        auto fragment::at () const noexcept -> typename enum_to_section<Key>::type const & {
            assert (has_fragment (Key));
            return offset_to_instance<typename enum_to_section<Key>::type const> (
                arr_[static_cast<std::size_t> (Key)]);
        }

        // check_range_is_sorted
        // ~~~~~~~~~~~~~~~~~~~~~
        template <typename Iterator>
        void fragment::check_range_is_sorted (Iterator first, Iterator last) {
            (void) first;
            (void) last;
            using value_type = typename std::iterator_traits<Iterator>::value_type;
            assert (
                std::is_sorted (first, last, [](value_type const & a, value_type const & b) {
                    return static_cast<unsigned> (a.type) < static_cast<unsigned> (b.type);
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
            std::for_each (first, last, [&size_bytes](fragment_data const & c) {
                size_bytes = c.aligned_size_t (size_bytes);
                size_bytes += c.size_bytes ();
            });
            return size_bytes;
        }


        // fragment::iterator
        // ~~~~~~~~~~~~~~~~~~
        inline fragment::const_iterator::const_iterator (fragment_type type,
                                                         wrapped_iterator it) noexcept
                : fragment_{type}
                , it_{it} {}

        inline fragment::const_iterator::const_iterator (wrapped_iterator it) noexcept
                : const_iterator (static_cast<fragment_type> (*it), it) {
            assert (*it < static_cast<std::uint64_t> (fragment_type::last));
        }

        inline auto fragment::const_iterator::operator* () const noexcept -> reference {
            return fragment_;
        }
        inline auto fragment::const_iterator::operator-> () const noexcept -> pointer {
            return &fragment_;
        }

        inline fragment::const_iterator & fragment::const_iterator::operator++ () noexcept {
            ++it_;
            fragment_ = static_cast<fragment_type> (*it_);
            return *this;
        }
        inline fragment::const_iterator fragment::const_iterator::operator++ (int) noexcept {
            auto const prev = *this;
            ++*this;
            return prev;
        }

        /// Returns the alignment of the given section type in the given fragment.
        unsigned section_align (fragment const & fragment, section_type type);

        /// Returns the number of bytes of storage that are accupied by the section with the
        /// given type in the given fragment.
        std::size_t section_size (fragment const & fragment, section_type type);

        /// Returns the ifixups of the given section type in the given fragment.
        section::container<internal_fixup> section_ifixups (fragment const & fragment,
                                                            section_type type);

        /// Returns the xfixups of the given section type in the given fragment.
        section::container<external_fixup> section_xfixups (fragment const & fragment,
                                                            section_type Type);

        /// Returns the section content of the given section type in the given fragment.
        section::container<std::uint8_t> section_value (fragment const & fragment,
                                                        section_type type);
    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_FRAGMENT_HPP
