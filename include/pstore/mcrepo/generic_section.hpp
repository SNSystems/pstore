//*                             _                      _   _              *
//*   __ _  ___ _ __   ___ _ __(_) ___   ___  ___  ___| |_(_) ___  _ __   *
//*  / _` |/ _ \ '_ \ / _ \ '__| |/ __| / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* | (_| |  __/ | | |  __/ |  | | (__  \__ \  __/ (__| |_| | (_) | | | | *
//*  \__, |\___|_| |_|\___|_|  |_|\___| |___/\___|\___|\__|_|\___/|_| |_| *
//*  |___/                                                                *
//===- include/pstore/mcrepo/generic_section.hpp --------------------------===//
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
#ifndef PSTORE_MCREPO_GENERIC_SECTION_HPP
#define PSTORE_MCREPO_GENERIC_SECTION_HPP

#include <algorithm>
#include <cstdint>
#include <type_traits>

#include "pstore/core/address.hpp"
#include "pstore/mcrepo/section.hpp"
#include "pstore/support/aligned.hpp"
#include "pstore/support/bit_count.hpp"
#include "pstore/support/small_vector.hpp"

namespace pstore {
    class indirect_string;

    namespace repo {

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
            // FIXME: much padding here.
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


        //*                        _                 _   _           *
        //*  __ _ ___ _ _  ___ _ _(_)__   ___ ___ __| |_(_)___ _ _   *
        //* / _` / -_) ' \/ -_) '_| / _| (_-</ -_) _|  _| / _ \ ' \  *
        //* \__, \___|_||_\___|_| |_\__| /__/\___\__|\__|_\___/_||_| *
        //* |___/                                                    *
        class generic_section {
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
            generic_section (DataRange const & d, IFixupRange const & i, XFixupRange const & x,
                             std::uint8_t align);

            template <typename DataRange, typename IFixupRange, typename XFixupRange>
            generic_section (sources<DataRange, IFixupRange, XFixupRange> const & src,
                             std::uint8_t align)
                    : generic_section (src.data_range, src.ifixups_range, src.xfixups_range,
                                       align) {}

            generic_section (generic_section const &) = delete;
            generic_section & operator= (generic_section const &) = delete;
            generic_section (generic_section &&) = delete;
            generic_section & operator= (generic_section &&) = delete;

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
            template <typename IntType, typename Iterator,
                      typename = typename std::enable_if<std::is_unsigned<IntType>::value>::type>
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
        generic_section::generic_section (DataRange const & d, IFixupRange const & i,
                                          XFixupRange const & x, std::uint8_t align)
                : align_{static_cast<std::uint8_t> (bit_count::ctz (align))}
                , num_ifixups_{0} {

            static_assert (std::is_standard_layout<generic_section>::value,
                           "section must satisfy StandardLayoutType");
            static_assert (offsetof (generic_section, align_) == 0, "align_ offset is not 0");
            static_assert (offsetof (generic_section, num_ifixups_) == 1,
                           "num_ifixups_ offset is not 1");
            static_assert (offsetof (generic_section, num_xfixups_) == 4,
                           "num_xfixups_ offset differs from expected value");
            static_assert (offsetof (generic_section, data_size_) == 8,
                           "data_size_ offset differs from expected value");
            static_assert (sizeof (generic_section) == 16, "section size does not match expected");
#ifndef NDEBUG
            auto const start = reinterpret_cast<std::uint8_t *> (this);
#endif
            auto p = reinterpret_cast<std::uint8_t *> (this + 1);
            assert (bit_count::pop_count (align) == 1);

            if (d.first != d.second) {
                p = std::copy (d.first, d.second, aligned_ptr<std::uint8_t> (p));
                data_size_ = generic_section::set_size<decltype (data_size_)> (d.first, d.second);
            }
            if (i.first != i.second) {
                p = reinterpret_cast<std::uint8_t *> (
                    std::copy (i.first, i.second, aligned_ptr<internal_fixup> (p)));
                this->set_num_ifixups (i.first, i.second, &num_ifixups_[0]);
            }
            if (x.first != x.second) {
                p = reinterpret_cast<std::uint8_t *> (
                    std::copy (x.first, x.second, aligned_ptr<external_fixup> (p)));
                num_xfixups_ =
                    generic_section::set_size<decltype (num_xfixups_)> (x.first, x.second);
            }
            assert (p >= start && static_cast<std::size_t> (p - start) == size_bytes (d, i, x));
        }

        // set_size
        // ~~~~~~~~
        template <typename IntType, typename Iterator, typename>
        inline IntType generic_section::set_size (Iterator first, Iterator last) {
            auto const size = std::distance (first, last);
            if (size <= 0) {
                return 0;
            }
            using common =
                typename std::common_type<IntType,
                                          typename std::make_unsigned<decltype (size)>::type>::type;
            return static_cast<IntType> (
                std::min (static_cast<common> (size),
                          static_cast<common> (std::numeric_limits<IntType>::max ())));
        }

        // size_bytes
        // ~~~~~~~~~~
        template <typename DataRange, typename IFixupRange, typename XFixupRange>
        std::size_t generic_section::size_bytes (DataRange const & d, IFixupRange const & i,
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
        inline std::uint32_t generic_section::num_ifixups () const noexcept {
            static_assert (sizeof (num_ifixups_) == 3, "num_ifixups is expected to be 3 bytes");
            return three_byte_integer::get (num_ifixups_);
        }

        // set_num_ifixups
        // ~~~~~~~~~~~~~~~
        template <typename Iterator>
        inline void generic_section::set_num_ifixups (Iterator first, Iterator last,
                                                      std::uint8_t * out) {
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
                -> generic_section::sources<range<decltype (data)::const_iterator>,
                                            range<decltype (ifixups)::const_iterator>,
                                            range<decltype (xfixups)::const_iterator>> {

                return generic_section::make_sources (
                    make_range (std::begin (data), std::end (data)),
                    make_range (std::begin (ifixups), std::end (ifixups)),
                    make_range (std::begin (xfixups), std::end (xfixups)));
            }
        };

        //*                  _   _               _ _               _      _             *
        //*  __ _ _ ___ __ _| |_(_)___ _ _    __| (_)____ __  __ _| |_ __| |_  ___ _ _  *
        //* / _| '_/ -_) _` |  _| / _ \ ' \  / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_| *
        //* \__|_| \___\__,_|\__|_\___/_||_| \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_|   *
        //*                                            |_|                              *
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


        class section_dispatcher final : public dispatcher {
        public:
            explicit section_dispatcher (generic_section const & s) noexcept
                    : s_{s} {}

            std::size_t size_bytes () const final { return s_.size_bytes (); }
            unsigned align () const final { return s_.align (); }
            std::size_t size () const final { return s_.data ().size (); }
            container<internal_fixup> ifixups () const final { return s_.ifixups (); }
            container<external_fixup> xfixups () const final { return s_.xfixups (); }
            container<std::uint8_t> data () const final { return s_.data (); }

        private:
            generic_section const & s_;
        };


        template <>
        struct section_to_dispatcher<generic_section> {
            using type = section_dispatcher;
        };

    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_GENERIC_SECTION_HPP