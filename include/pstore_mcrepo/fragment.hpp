//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===- include/pstore_mcrepo/fragment.hpp ---------------------------------===//
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

#include "pstore/address.hpp"
#include "pstore/file_header.hpp"
#include "pstore/transaction.hpp"
#include "pstore_mcrepo/aligned.hpp"
#include "pstore_mcrepo/sparse_array.hpp"
#include "pstore_support/small_vector.hpp"

namespace pstore {
    namespace repo {

        //*  _     _                     _    __ _                *
        //* (_)_ _| |_ ___ _ _ _ _  __ _| |  / _(_)_ ___  _ _ __  *
        //* | | ' \  _/ -_) '_| ' \/ _` | | |  _| \ \ / || | '_ \ *
        //* |_|_||_\__\___|_| |_||_\__,_|_| |_| |_/_\_\\_,_| .__/ *
        //*                                                |_|    *
        struct internal_fixup {
            std::uint8_t section;
            std::uint8_t type;
            std::uint16_t padding;
            std::uint32_t offset;
            std::uint32_t addend;
        };

        static_assert (std::is_standard_layout<internal_fixup>::value,
                       "internal_fixup must satisfy StandardLayoutType");

        static_assert (offsetof (internal_fixup, section) == 0,
                       "section offset differs from expected value");
        static_assert (offsetof (internal_fixup, type) == 1,
                       "type offset differs from expected value");
        static_assert (offsetof (internal_fixup, padding) == 2,
                       "padding offset differs from expected value");
        static_assert (offsetof (internal_fixup, offset) == 4,
                       "offset offset differs from expected value");
        static_assert (offsetof (internal_fixup, addend) == 8,
                       "addend offset differs from expected value");
        static_assert (sizeof (internal_fixup) == 12,
                       "internal_fixup size does not match expected");

        //*          _                     _    __ _                *
        //*  _____ _| |_ ___ _ _ _ _  __ _| |  / _(_)_ ___  _ _ __  *
        //* / -_) \ /  _/ -_) '_| ' \/ _` | | |  _| \ \ / || | '_ \ *
        //* \___/_\_\\__\___|_| |_||_\__,_|_| |_| |_/_\_\\_,_| .__/ *
        //*                                                  |_|    *
        struct external_fixup {
            pstore::address name;
            std::uint8_t type;
            // FIXME: much padding here.
            std::uint64_t offset;
            std::uint64_t addend;
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
        // static_assert (offsetof (external_fixup, padding3) == 20, "padding3 offset
        // differs from expected value");
        static_assert (sizeof (external_fixup) == 32,
                       "external_fixup size does not match expected");

        //*             _   _           *
        //*  ___ ___ __| |_(_)___ _ _   *
        //* (_-</ -_) _|  _| / _ \ ' \  *
        //* /__/\___\__|\__|_\___/_||_| *
        //*                             *
        class section {
            friend struct section_check;

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
            section (DataRange const & d, IFixupRange const & i, XFixupRange const & x);

            template <typename DataRange, typename IFixupRange, typename XFixupRange>
            section (sources<DataRange, IFixupRange, XFixupRange> const & src)
                    : section (src.data_range, src.ifixups_range, src.xfixups_range) {}

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
                iterator begin () const {
                    return begin_;
                }
                iterator end () const {
                    return end_;
                }
                const_iterator cbegin () const {
                    return begin ();
                }
                const_iterator cend () const {
                    return end ();
                }

                size_type size () const {
                    return end_ - begin_;
                }

            private:
                const_pointer begin_;
                const_pointer end_;
            };

            container<std::uint8_t> data () const {
                auto begin = aligned_ptr<std::uint8_t> (this + 1);
                return {begin, begin + data_size_};
            }
            container<internal_fixup> ifixups () const {
                auto begin = aligned_ptr<internal_fixup> (data ().end ());
                return {begin, begin + num_ifixups_};
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
            std::uint32_t num_ifixups_ = 0;
            std::uint32_t num_xfixups_ = 0;
            std::uint64_t data_size_ = 0;

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

        static_assert (std::is_standard_layout<section>::value,
                       "section must satisfy StandardLayoutType");

        /// A trivial class which exists solely to verify the layout of the section type
        /// (including its private member variables and is declared as a friend of it.
        struct section_check {
            static_assert (offsetof (section, num_ifixups_) == 0,
                           "num_ifixups_ offset differs from expected value");
            static_assert (offsetof (section, num_xfixups_) == 4,
                           "num_xfixups_ offset differs from expected value");
            static_assert (offsetof (section, data_size_) == 8,
                           "data_size_ offset differs from expected value");
            static_assert (sizeof (section) == 16, "section size does not match expected");
        };

        // (ctor)
        // ~~~~~~
        template <typename DataRange, typename IFixupRange, typename XFixupRange>
        section::section (DataRange const & d, IFixupRange const & i, XFixupRange const & x) {
            auto const start = reinterpret_cast<std::uint8_t *> (this);
            auto p = reinterpret_cast<std::uint8_t *> (this + 1);

            if (d.first != d.second) {
                p = std::copy (d.first, d.second, aligned_ptr<std::uint8_t> (p));
                data_size_ = section::set_size<decltype (data_size_)> (d.first, d.second);
            }
            if (i.first != i.second) {
                p = reinterpret_cast<std::uint8_t *> (
                    std::copy (i.first, i.second, aligned_ptr<internal_fixup> (p)));
                num_ifixups_ = section::set_size<decltype (num_ifixups_)> (i.first, i.second);
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

            auto const usize =
                static_cast<typename std::make_unsigned<decltype (size)>::type> (size);
            assert (usize >= std::numeric_limits<IntType>::min ());
            assert (usize <= std::numeric_limits<IntType>::max ());

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

// FIXME: the members of this collection are drawn from
// RepoObjectWriter::writeRepoSectionData(). Not sure it's correct.
#define PSTORE_REPO_SECTION_TYPES                                                                  \
    X (BSS)                                                                                        \
    X (Common)                                                                                     \
    X (Data)                                                                                       \
    X (RelRo)                                                                                      \
    X (Text)                                                                                       \
    X (Mergeable1ByteCString)                                                                      \
    X (Mergeable2ByteCString)                                                                      \
    X (Mergeable4ByteCString)                                                                      \
    X (MergeableConst4)                                                                            \
    X (MergeableConst8)                                                                            \
    X (MergeableConst16)                                                                           \
    X (MergeableConst32)                                                                           \
    X (MergeableConst)                                                                             \
    X (ReadOnly)                                                                                   \
    X (ThreadBSS)                                                                                  \
    X (ThreadData)                                                                                 \
    X (ThreadLocal)                                                                                \
    X (Metadata)

#define X(a) a,
        enum class section_type : std::uint8_t { PSTORE_REPO_SECTION_TYPES };
#undef X

        struct section_content {
            section_content () = default;
            section_content (section_content const &) = default;
            section_content (section_content &&) = default;
            section_content & operator= (section_content const &) = default;
            section_content & operator= (section_content &&) = default;

            template <typename Iterator>
            using range = std::pair<Iterator, Iterator>;

            template <typename Iterator>
            static inline auto make_range (Iterator begin, Iterator end) -> range<Iterator> {
                return {begin, end};
            }

            explicit section_content (section_type st)
                    : type{st} {}

            section_type type;
            small_vector<char, 128> data;
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

            /// An iterator adaptor which produces a value_type of 'section_type const' from
            /// values deferences from the supplied underlying iterator.
            template <typename Iterator>
            class content_type_iterator {
            public:
                using value_type = section_type const;
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
                bool operator== (content_type_iterator const & rhs) const {
                    return it_ == rhs.it_;
                }
                bool operator!= (content_type_iterator const & rhs) const {
                    return !(operator== (rhs));
                }
                content_type_iterator & operator++ () {
                    ++it_;
                    return *this;
                }
                content_type_iterator operator++ (int) {
                    content_type_iterator old{*this};
                    it_++;
                    return old;
                }

                reference operator* () const {
                    return (*it_).type;
                }
                pointer operator-> () const {
                    return &(*it_).type;
                }
                reference operator[] (difference_type n) const {
                    return it_[n].type;
                }

            private:
                Iterator it_;
            };

            template <typename Iterator>
            inline content_type_iterator<Iterator> make_content_type_iterator (Iterator it) {
                return content_type_iterator<Iterator> (it);
            }

            /// An iterator adaptor which produces a value_type of dereferences the
            /// value_type of the wrapped iterator.
            template <typename Iterator>
            class section_content_iterator {
            public:
                using value_type = typename std::pointer_traits<
                    typename std::pointer_traits<Iterator>::element_type>::element_type;
                using difference_type = typename std::iterator_traits<Iterator>::difference_type;
                using pointer = value_type *;
                using reference = value_type &;
                using iterator_category = std::input_iterator_tag;

                section_content_iterator ()
                        : it_{} {}
                explicit section_content_iterator (Iterator it)
                        : it_{it} {}
                section_content_iterator (section_content_iterator const & rhs)
                        : it_{rhs.it_} {}
                section_content_iterator & operator= (section_content_iterator const & rhs) {
                    it_ = rhs.it_;
                    return *this;
                }
                bool operator== (section_content_iterator const & rhs) const {
                    return it_ == rhs.it_;
                }
                bool operator!= (section_content_iterator const & rhs) const {
                    return !(operator== (rhs));
                }
                section_content_iterator & operator++ () {
                    ++it_;
                    return *this;
                }
                section_content_iterator operator++ (int) {
                    section_content_iterator old{*this};
                    it_++;
                    return old;
                }

                reference operator* () const {
                    return **it_;
                }
                pointer operator-> () const {
                    return &(**it_);
                }
                reference operator[] (difference_type n) const {
                    return *(it_[n]);
                }

            private:
                Iterator it_;
            };

            template <typename Iterator>
            inline section_content_iterator<Iterator> make_section_content_iterator (Iterator it) {
                return section_content_iterator<Iterator> (it);
            }

        } // end namespace details

        //*   __                             _    *
        //*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_  *
        //* |  _| '_/ _` / _` | '  \/ -_) ' \  _| *
        //* |_| |_| \__,_\__, |_|_|_\___|_||_\__| *
        //*              |___/                    *
        class fragment {
        public:
            struct deleter {
                void operator() (void * p);
            };

            template <typename TransactionType, typename Iterator>
            static auto alloc (TransactionType & transaction, Iterator first, Iterator last)
                -> pstore::record;

            template <typename Iterator>
            void populate (Iterator first, Iterator last);

            using member_array = sparse_array<std::uint64_t>;

            section const & operator[] (section_type key) const;
            std::size_t num_sections () const {
                return arr_.size ();
            }
            member_array const & sections () const {
                return arr_;
            }

            /// Returns the number of bytes of storage that is required for a fragment containing
            /// the sections defined by [first, last).
            template <typename Iterator>
            static std::size_t size_bytes (Iterator first, Iterator last);

        private:
            template <typename IteratorIdx>
            fragment (IteratorIdx first_index, IteratorIdx last_index)
                    : arr_ (first_index, last_index) {}

            section const & offset_to_section (std::uint64_t offset) const {
                auto ptr = reinterpret_cast<std::uint8_t const *> (this) + offset;
                assert (reinterpret_cast<std::uintptr_t> (ptr) % alignof (section) == 0);
                return *reinterpret_cast<section const *> (ptr);
            }

            member_array arr_;
        };


#define X(a)                                                                                       \
    case (section_type::a):                                                                        \
        name = #a;                                                                                 \
        break;

        // operator<<
        // ~~~~~~~~~~
        template <typename OStream>
        OStream & operator<< (OStream & os, section_type st) {
            char const * name = "*unknown*";
            switch (st) { PSTORE_REPO_SECTION_TYPES }
            return os << name;
        }
#undef X

        template <typename OStream>
        OStream & operator<< (OStream & os, internal_fixup const & ifx) {
            using TypeT = typename std::common_type<unsigned, decltype (ifx.type)>::type;
            return os << "{section:" << static_cast<unsigned> (ifx.section)
                      << ",type:" << static_cast<TypeT> (ifx.type) << ",offset:" << ifx.offset
                      << ",addend:" << ifx.addend << "}";
        }

        template <typename OStream>
        OStream & operator<< (OStream & os, external_fixup const & xfx) {
            using TypeT = typename std::common_type<unsigned, decltype (xfx.type)>::type;
            return os << R"({name:")" << xfx.name.absolute () << R"(",type:)"
                      << static_cast<TypeT> (xfx.type) << ",offset:" << xfx.offset
                      << ",addend:" << xfx.addend << "}";
        }

        template <typename OStream>
        OStream & operator<< (OStream & os, section const & scn) {
            auto digit_to_hex = [](unsigned v) {
                assert (v < 0x10);
                return static_cast<char> (v + ((v < 10) ? '0' : 'a' - 10));
            };

            char const * indent = "\n  ";
            os << '{' << indent << "data: ";
            for (uint8_t v : scn.data ()) {
                os << "0x" << digit_to_hex (v >> 4) << digit_to_hex (v & 0xF) << ',';
            }
            os << indent << "ifixups: [ ";
            for (auto const & ifixup : scn.ifixups ()) {
                os << ifixup << ", ";
            }
            os << ']' << indent << "xfixups: [ ";
            for (auto const & xfixup : scn.xfixups ()) {
                os << xfixup << ", ";
            }
            return os << "]\n}";
        }

        template <typename OStream>
        OStream & operator<< (OStream & os, fragment const & frag) {
            for (auto const key : frag.sections ().get_indices ()) {
                auto const type = static_cast<section_type> (key);
                os << type << ": " << frag[type] << '\n';
            }
            return os;
        }

        // alloc
        // ~~~~~
        template <typename TransactionType, typename Iterator>
        auto fragment::alloc (TransactionType & transaction, Iterator first, Iterator last)
            -> pstore::record {
            static_assert ((std::is_same<typename std::iterator_traits<Iterator>::value_type,
                                         section_content>::value),
                           "Iterator value_type should be section_content");

            // Compute the number of bytes of storage that we'll need for this fragment.
            auto size = fragment::size_bytes (first, last);

            // Allocate storage for the fragment including its three arrays.
            std::pair<std::shared_ptr<void>, pstore::address> storage =
                transaction.alloc_rw (size, alignof (fragment));
            auto ptr = storage.first;

            // Construct the basic fragment structure into this memory.
            auto fragment_ptr =
                new (ptr.get ()) fragment (details::make_content_type_iterator (first),
                                           details::make_content_type_iterator (last));
            // Point past the end of the sparse array.
            auto out =
                reinterpret_cast<std::uint8_t *> (fragment_ptr) + fragment_ptr->arr_.size_bytes ();

            // Copy the contents of each of the segments to the fragment.
            std::for_each (first, last, [&out, fragment_ptr](section_content const & c) {
                out = reinterpret_cast<std::uint8_t *> (aligned_ptr<section> (out));
                auto scn = new (out) section (c.make_sources ());
                auto offset = reinterpret_cast<std::uintptr_t> (scn) -
                              reinterpret_cast<std::uintptr_t> (fragment_ptr);
                fragment_ptr->arr_[static_cast<unsigned> (c.type)] = offset;
                out += scn->size_bytes ();
            });

            assert (out >= ptr.get () &&
                    static_cast<std::size_t> (
                        out - reinterpret_cast<std::uint8_t *> (ptr.get ())) == size);
            return {storage.second, size};
        }

        // populate
        // ~~~~~~~~
        template <typename Iterator>
        void fragment::populate (Iterator first, Iterator last) {
            // Point past the end of the sparse array.
            auto out = reinterpret_cast<std::uint8_t *> (this) + arr_.size_bytes ();

            // Copy the contents of each of the segments to the fragment.
            std::for_each (first, last, [&out, this](section_content const & c) {
                out = reinterpret_cast<std::uint8_t *> (aligned_ptr<section> (out));
                auto scn = new (out) section (c.make_sources ());
                auto offset = reinterpret_cast<std::uintptr_t> (scn) -
                              reinterpret_cast<std::uintptr_t> (this);
                arr_[static_cast<unsigned> (c.type)] = offset;
                out += scn->size_bytes ();
            });
        }

        // size_bytes
        // ~~~~~~~~~~
        template <typename Iterator>
        std::size_t fragment::size_bytes (Iterator first, Iterator last) {
            auto const num_sections = std::distance (first, last);
            assert (num_sections >= 0);

            // Space needed by the section offset array.
            std::size_t size_bytes = decltype (fragment::arr_)::size_bytes (num_sections);
            // Now the storage for each of the sections
            std::for_each (first, last, [&size_bytes](section_content const & c) {
                size_bytes = aligned<section> (size_bytes);
                size_bytes += section::size_bytes (c.make_sources ());
            });
            return size_bytes;
        }

    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_FRAGMENT_HPP
// eof: include/pstore_mcrepo/fragment.hpp
