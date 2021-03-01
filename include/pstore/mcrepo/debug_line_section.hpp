//===- include/pstore/mcrepo/debug_line_section.hpp -------*- mode: C++ -*-===//
//*      _      _                   _ _             *
//*   __| | ___| |__  _   _  __ _  | (_)_ __   ___  *
//*  / _` |/ _ \ '_ \| | | |/ _` | | | | '_ \ / _ \ *
//* | (_| |  __/ |_) | |_| | (_| | | | | | | |  __/ *
//*  \__,_|\___|_.__/ \__,_|\__, | |_|_|_| |_|\___| *
//*                         |___/                   *
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
/// \file debug_line_section.hpp
/// \brief  Declares the type used to record debug_line sections in a fragment.

#ifndef PSTORE_MCREPO_DEBUG_LINE_SECTION_HPP
#define PSTORE_MCREPO_DEBUG_LINE_SECTION_HPP

#include "pstore/core/address.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/mcrepo/generic_section.hpp"

namespace pstore {
    namespace repo {

        // TODO: modeling the debug line section as a "generic_section plus an extent for the CU
        // header" is expedient but we probably don't need to store the alignment or external fixup
        // array.
        class debug_line_section : public section_base {
        public:
            template <typename DataRange, typename IFixupRange, typename XFixupRange>
            debug_line_section (
                index::digest const & header_digest, extent<std::uint8_t> const & header_extent,
                generic_section::sources<DataRange, IFixupRange, XFixupRange> const & src,
                std::uint8_t align)
                    : header_digest_{header_digest}
                    , header_{header_extent}
                    , g_{src.data_range, src.ifixups_range, src.xfixups_range, align} {}


            index::digest const & header_digest () const noexcept { return header_digest_; }
            extent<std::uint8_t> const & header_extent () const noexcept { return header_; }
            generic_section const & generic () const noexcept { return g_; }

            unsigned align () const noexcept { return g_.align (); }
            // \returns The section's data payload.
            container<std::uint8_t> payload () const { return g_.payload (); }
            /// \returns The number of bytes in the section's data payload.
            std::size_t size () const noexcept { return this->payload ().size (); }
            container<internal_fixup> ifixups () const { return g_.ifixups (); }
            container<external_fixup> xfixups () const { return g_.xfixups (); }

            /// \returns The number of bytes occupied by this section.
            std::size_t size_bytes () const {
                return offsetof (debug_line_section, g_) + g_.size_bytes ();
            }

            template <typename DataRange, typename IFixupRange, typename XFixupRange>
            static std::size_t size_bytes (DataRange const & d, IFixupRange const & i,
                                           XFixupRange const & x) {
                return offsetof (debug_line_section, g_) + generic_section::size_bytes (d, i, x);
            }

            template <typename DataRange, typename IFixupRange, typename XFixupRange>
            static std::size_t
            size_bytes (generic_section::sources<DataRange, IFixupRange, XFixupRange> const & src) {
                return size_bytes (src.data_range, src.ifixups_range, src.xfixups_range);
            }

        private:
            index::digest header_digest_;
            extent<std::uint8_t> header_;
            generic_section g_;
        };
        static_assert (std::is_standard_layout<debug_line_section>::value,
                       "debug_line_section must be standard-layout");
        static_assert (alignof (debug_line_section) == 16,
                       "debug_line_section alignment must be 16");


        template <>
        inline unsigned section_alignment<pstore::repo::debug_line_section> (
            pstore::repo::debug_line_section const & s) noexcept {
            return s.align ();
        }
        template <>
        inline std::uint64_t section_size<pstore::repo::debug_line_section> (
            pstore::repo::debug_line_section const & s) noexcept {
            return s.size ();
        }


        class debug_line_section_creation_dispatcher final : public section_creation_dispatcher {
        public:
            debug_line_section_creation_dispatcher (index::digest const & header_digest,
                                                    extent<std::uint8_t> const & header,
                                                    section_content const * const sec)
                    : section_creation_dispatcher (section_kind::debug_line)
                    , header_digest_{header_digest}
                    , header_{header}
                    , section_{sec} {}

            debug_line_section_creation_dispatcher (
                debug_line_section_creation_dispatcher const &) = delete;
            debug_line_section_creation_dispatcher &
            operator= (debug_line_section_creation_dispatcher const &) = delete;

            std::size_t size_bytes () const override;

            // Write the section data to the memory pointed to by \p out.
            std::uint8_t * write (std::uint8_t * out) const final;

        private:
            std::uintptr_t aligned_impl (std::uintptr_t in) const override;

            index::digest header_digest_;
            extent<std::uint8_t> header_;
            section_content const * const section_;
        };

        template <>
        struct section_to_creation_dispatcher<debug_line_section> {
            using type = debug_line_section_creation_dispatcher;
        };

        class debug_line_dispatcher final : public dispatcher {
        public:
            explicit constexpr debug_line_dispatcher (debug_line_section const & d) noexcept
                    : d_{d} {}
            ~debug_line_dispatcher () noexcept final;

            std::size_t size_bytes () const final { return d_.size_bytes (); }
            unsigned align () const final { return d_.align (); }
            std::size_t size () const final { return d_.size (); }
            container<internal_fixup> ifixups () const final { return d_.ifixups (); }
            container<external_fixup> xfixups () const final { return d_.xfixups (); }
            container<std::uint8_t> payload () const final { return d_.payload (); }

        private:
            debug_line_section const & d_;
        };


        template <>
        struct section_to_dispatcher<debug_line_section> {
            using type = debug_line_dispatcher;
        };

    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_DEBUG_LINE_SECTION_HPP
