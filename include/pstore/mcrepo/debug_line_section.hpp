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
//===- include/pstore/mcrepo/debug_line_section.hpp -----------------------===//
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
#ifndef PSTORE_MCREPO_DEBUG_LINE_SECTION_HPP
#define PSTORE_MCREPO_DEBUG_LINE_SECTION_HPP

#include "pstore/mcrepo/generic_section.hpp"
#include "pstore/core/file_header.hpp" // TODO: extent shouldn't be in file_header.hpp!

namespace pstore {
    namespace repo {

        // TODO: modelling the debug line section as a "generic_section plus an extent for the CU
        // header is expedient but we probably don't need to store the alignment or xfixup array.
        class debug_line_section : public section_base {
        public:
            template <typename DataRange, typename IFixupRange, typename XFixupRange>
            debug_line_section (
                extent<std::uint8_t> const & header_extent,
                generic_section::sources<DataRange, IFixupRange, XFixupRange> const & src,
                std::uint8_t align)
                    : header_{header_extent}
                    , g_ (src.data_range, src.ifixups_range, src.xfixups_range, align) {}


            extent<std::uint8_t> const & header_extent () const noexcept { return header_; }
            generic_section const & generic () const noexcept { return g_; }

            /// \returns The number of bytes occupied by this section.
            std::size_t size_bytes () const {
                return offsetof (debug_line_section, g_) + g_.size_bytes ();
            }

            unsigned align () const noexcept { return g_.align (); }
            // \returns The section's data payload.
            container<std::uint8_t> data () const { return g_.data (); }
            /// \returns The number of bytes in the section's data payload.
            std::size_t size () const { return g_.data ().size (); }
            container<internal_fixup> ifixups () const { return g_.ifixups (); }
            container<external_fixup> xfixups () const { return g_.xfixups (); }

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
            extent<std::uint8_t> header_;
            generic_section g_;
        };
        static_assert (std::is_standard_layout<debug_line_section>::value,
                       "debug_line_section must be standard-layout");


        class debug_line_section_creation_dispatcher final : public section_creation_dispatcher {
        public:
            debug_line_section_creation_dispatcher (extent<std::uint8_t> const & header,
                                                    section_content const * sec)
                    : section_creation_dispatcher (section_kind::debug_line)
                    , header_{header}
                    , section_ (sec) {}

            debug_line_section_creation_dispatcher (
                debug_line_section_creation_dispatcher const &) = delete;
            debug_line_section_creation_dispatcher &
            operator= (debug_line_section_creation_dispatcher const &) = delete;

            std::size_t size_bytes () const override;

            // Write the section data to the memory pointed to by \p out.
            std::uint8_t * write (std::uint8_t * out) const final;

        private:
            std::uintptr_t aligned_impl (std::uintptr_t in) const override;
            extent<std::uint8_t> header_;
            section_content const * const section_;
        };


        class debug_line_dispatcher final : public dispatcher {
        public:
            explicit debug_line_dispatcher (debug_line_section const & d) noexcept
                    : d_{d} {}
            ~debug_line_dispatcher () noexcept;

            std::size_t size_bytes () const final { return d_.size_bytes (); }
            unsigned align () const final { return d_.align (); }
            std::size_t size () const final { return d_.size (); }
            container<internal_fixup> ifixups () const final { return d_.ifixups (); }
            container<external_fixup> xfixups () const final { return d_.xfixups (); }
            container<std::uint8_t> data () const final { return d_.data (); }

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
