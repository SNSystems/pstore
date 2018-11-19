//*  _                             _   _              *
//* | |__  ___ ___   ___  ___  ___| |_(_) ___  _ __   *
//* | '_ \/ __/ __| / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* | |_) \__ \__ \ \__ \  __/ (__| |_| | (_) | | | | *
//* |_.__/|___/___/ |___/\___|\___|\__|_|\___/|_| |_| *
//*                                                   *
//===- include/pstore/mcrepo/bss_section.hpp ------------------------------===//
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
#ifndef PSTORE_MCREPO_BSS_SECTION_HPP
#define PSTORE_MCREPO_BSS_SECTION_HPP

#include "pstore/mcrepo/generic_section.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace repo {

        //*  _                       _   _           *
        //* | |__ ______  ___ ___ __| |_(_)___ _ _	 *
        //* | '_ (_-<_-< (_-</ -_) _|  _| / _ \ ' \	 *
        //* |_.__/__/__/ /__/\___\__|\__|_\___/_||_| *
        //* 										 *
        class bss_section : public section_base {
        public:
            bss_section (std::uint8_t align, std::uint64_t size)
                    : align_{static_cast<std::uint8_t> (bit_count::ctz (align))}
                    , size_{size} {
                (void) padding_;
                PSTORE_STATIC_ASSERT (std::is_standard_layout<bss_section>::value);
                PSTORE_STATIC_ASSERT (offsetof (bss_section, align_) == 0);
                PSTORE_STATIC_ASSERT (offsetof (bss_section, size_) == 8);
                PSTORE_STATIC_ASSERT (alignof (bss_section) == 8);
            }

            bss_section (bss_section const &) = delete;
            bss_section & operator= (bss_section const &) = delete;
            bss_section (bss_section &&) = delete;
            bss_section & operator= (bss_section &&) = delete;

            unsigned align () const noexcept { return 1U << align_; }
            std::uint64_t size () const noexcept { return size_; }

            container<internal_fixup> ifixups () const { return {}; }
            container<external_fixup> xfixups () const { return {}; }

            /// Returns the number of bytes occupied by this section.
            std::size_t size_bytes () const noexcept { return sizeof (bss_section); }

        private:
            /// The alignment of this section expressed as a power of two (i.e. 8 byte alignment is
            /// expressed as an align_ value of 3).
            std::uint8_t align_;

            std::uint8_t padding_[3] = {0, 0, 0};

            /// The number of bytes in the BSS section's data payload.
            std::uint64_t size_;
        };

        //*                  _   _               _ _               _      _             *
        //*  __ _ _ ___ __ _| |_(_)___ _ _    __| (_)____ __  __ _| |_ __| |_  ___ _ _  *
        //* / _| '_/ -_) _` |  _| / _ \ ' \  / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_| *
        //* \__|_| \___\__,_|\__|_\___/_||_| \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_|   *
        //*                                            |_|                              *
        class bss_section_creation_dispatcher final : public section_creation_dispatcher {
        public:
            bss_section_creation_dispatcher (gsl::not_null<section_content const *> sec)
                    : section_creation_dispatcher (section_kind::bss)
                    , section_ (sec) {
                assert (sec->ifixups.empty () && sec->xfixups.empty ());
            }

            bss_section_creation_dispatcher (bss_section_creation_dispatcher const &) = delete;
            bss_section_creation_dispatcher &
            operator= (bss_section_creation_dispatcher const &) = delete;

            std::size_t size_bytes () const final;

            std::uint8_t * write (std::uint8_t * out) const final;

        private:
            std::uintptr_t aligned_impl (std::uintptr_t in) const final;
            section_content const * const section_;
        };


        class bss_section_dispatcher final : public dispatcher {
        public:
            explicit bss_section_dispatcher (bss_section const & b) noexcept
                    : b_{b} {}
            ~bss_section_dispatcher () noexcept override;

            std::size_t size_bytes () const final { return b_.size_bytes (); }
            unsigned align () const final { return b_.align (); }
            std::size_t size () const final { return b_.size (); }
            container<internal_fixup> ifixups () const final { return {}; }
            container<external_fixup> xfixups () const final { return {}; }
            container<std::uint8_t> data () const final { return {}; }

        private:
            bss_section const & b_;
        };


        template <>
        struct section_to_dispatcher<bss_section> {
            using type = bss_section_dispatcher;
        };

    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_MCREPO_BSS_SECTION_HPP
