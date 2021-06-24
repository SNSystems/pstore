//===- include/pstore/mcrepo/section.hpp ------------------*- mode: C++ -*-===//
//*                _   _              *
//*  ___  ___  ___| |_(_) ___  _ __   *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* \__ \  __/ (__| |_| | (_) | | | | *
//* |___/\___|\___|\__|_|\___/|_| |_| *
//*                                   *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_MCREPO_SECTION_HPP
#define PSTORE_MCREPO_SECTION_HPP

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iosfwd>
#include <type_traits>

#include "pstore/support/assert.hpp"

namespace pstore {
    namespace repo {

#define PSTORE_MCREPO_SECTION_KINDS                                                                \
    X (text)                                                                                       \
    X (data)                                                                                       \
    X (bss)                                                                                        \
    X (rel_ro)                                                                                     \
    X (mergeable_1_byte_c_string)                                                                  \
    X (mergeable_2_byte_c_string)                                                                  \
    X (mergeable_4_byte_c_string)                                                                  \
    X (mergeable_const_4)                                                                          \
    X (mergeable_const_8)                                                                          \
    X (mergeable_const_16)                                                                         \
    X (mergeable_const_32)                                                                         \
    X (read_only)                                                                                  \
    X (thread_data)                                                                                \
    X (thread_bss)                                                                                 \
    X (debug_line)                                                                                 \
    X (debug_string)                                                                               \
    X (debug_ranges)                                                                               \
    X (linked_definitions)

#define X(a) a,
        enum class section_kind : std::uint8_t {
            PSTORE_MCREPO_SECTION_KINDS last // always last, never used.
        };
#undef X
        constexpr auto num_section_kinds =
            static_cast<std::underlying_type<section_kind>::type> (section_kind::last);

        std::ostream & operator<< (std::ostream & os, section_kind kind);

        constexpr auto first_repo_metadata_section = section_kind::linked_definitions;

        constexpr bool is_target_section (section_kind const t) noexcept {
            using utype = std::underlying_type<section_kind>::type;
            return static_cast<utype> (t) < static_cast<utype> (first_repo_metadata_section);
        }


        /// An empty class used as the base type for all sections.
        struct section_base {};


        template <typename T>
        unsigned section_alignment (T const & section) noexcept;

        template <typename T>
        std::uint64_t section_size (T const & section) noexcept;

        //*                  _   _               _ _               _      _             *
        //*  __ _ _ ___ __ _| |_(_)___ _ _    __| (_)____ __  __ _| |_ __| |_  ___ _ _  *
        //* / _| '_/ -_) _` |  _| / _ \ ' \  / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_| *
        //* \__|_| \___\__,_|\__|_\___/_||_| \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_|   *
        //*                                            |_|                              *
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
            explicit section_creation_dispatcher (section_kind const kind) noexcept
                    : kind_{kind} {}
            section_creation_dispatcher (section_creation_dispatcher const &) = delete;
            section_creation_dispatcher (section_creation_dispatcher &&) = delete;
            virtual ~section_creation_dispatcher () noexcept;

            section_creation_dispatcher & operator= (section_creation_dispatcher const &) = delete;
            section_creation_dispatcher & operator= (section_creation_dispatcher &&) = delete;

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
            std::uint8_t * aligned (std::uint8_t * const a) const {
                return reinterpret_cast<std::uint8_t *> (
                    this->aligned_impl (reinterpret_cast<std::uintptr_t> (a)));
            }

            /// Returns the number of bytes of storage that are required for an instance of the
            /// section data.
            virtual std::size_t size_bytes () const = 0;

            /// Copies the section instance data to the memory starting at \p out. On entry, \p out
            /// is aligned according to the result of the aligned() member function.
            /// \param out  The address to which the instance data will be written.
            /// \returns The address past the end of instance data where the next section's data can
            /// be writen.
            virtual std::uint8_t * write (std::uint8_t * out) const = 0;

        private:
            /// \param v  The value to be aligned.
            /// \returns The value closest to but greater than or equal to \p v which is correctly
            /// aligned for an instance of the type used for an instance of this section kind.
            virtual std::uintptr_t aligned_impl (std::uintptr_t v) const = 0;

            section_kind const kind_;
        };

        /// Maps from the type of data that is associated with a fragment's section to a
        /// "dispatcher" subclass which provides a generic interface for creating instances of these
        /// sections.
        template <typename T>
        struct section_to_creation_dispatcher {};

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

            container () = default;
            container (const_pointer begin, const_pointer end)
                    : begin_{begin}
                    , end_{end} {
                PSTORE_ASSERT (end >= begin);
            }
            iterator begin () const { return begin_; }
            iterator end () const { return end_; }
            const_iterator cbegin () const { return begin (); }
            const_iterator cend () const { return end (); }

            const_pointer data () const { return begin_; }

            size_type size () const noexcept {
                PSTORE_ASSERT (end_ >= begin_);
                return static_cast<size_type> (end_ - begin_);
            }
            bool empty () const noexcept { return size () == 0U; }

        private:
            const_pointer begin_;
            const_pointer end_;
        };

        struct internal_fixup;
        struct external_fixup;

        /// This class is used to add virtual methods to a fragment's section. The section types
        /// themselves cannot be virtual because they're written to disk and wouldn't be portable
        /// between different C++ ABIs. The concrete classes derived from "dispatcher" wrap the real
        /// section data types and forward to calls directly to them.
        class dispatcher {
        public:
            virtual ~dispatcher () noexcept;

            virtual std::size_t size_bytes () const = 0;
            virtual unsigned align () const = 0;
            virtual std::size_t size () const = 0;
            virtual container<internal_fixup> ifixups () const = 0;
            virtual container<external_fixup> xfixups () const = 0;
            /// Return the data section stored in the object file. For example, the bss section has
            /// empty data section.
            virtual container<std::uint8_t> payload () const = 0;
        };


        /// Maps from the type of data that is associated with a fragment's section to a
        /// "dispatcher" subclass which provides a generic interface to the behavior of these
        /// sections.
        template <typename T>
        struct section_to_dispatcher {};

    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_SECTION_HPP
