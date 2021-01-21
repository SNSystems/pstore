//*  _                             _   _              *
//* | |__  ___ ___   ___  ___  ___| |_(_) ___  _ __   *
//* | '_ \/ __/ __| / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* | |_) \__ \__ \ \__ \  __/ (__| |_| | (_) | | | | *
//* |_.__/|___/___/ |___/\___|\___|\__|_|\___/|_| |_| *
//*                                                   *
//===- lib/mcrepo/bss_section.cpp -----------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/mcrepo/bss_section.hpp"

namespace pstore {
    namespace repo {

        //*                  _   _               _ _               _      _             *
        //*  __ _ _ ___ __ _| |_(_)___ _ _    __| (_)____ __  __ _| |_ __| |_  ___ _ _  *
        //* / _| '_/ -_) _` |  _| / _ \ ' \  / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_| *
        //* \__|_| \___\__,_|\__|_\___/_||_| \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_|   *
        //*                                            |_|                              *
        std::size_t bss_section_creation_dispatcher::size_bytes () const {
            return sizeof (bss_section);
        }

        std::uint8_t * bss_section_creation_dispatcher::write (std::uint8_t * const out) const {
            PSTORE_ASSERT (section_ != nullptr &&
                           "Must provide BSS section information before write");
            PSTORE_ASSERT (this->aligned (out) == out &&
                           "The target address must be properly aligned");
            PSTORE_ASSERT (section_->data.size () <=
                               std::numeric_limits<bss_section::size_type>::max () &&
                           "The BSS section is too large");

            auto * const scn = new (out) bss_section (
                section_->align, static_cast<bss_section::size_type> (section_->data.size ()));
            return out + scn->size_bytes ();
        }

        std::uintptr_t
        bss_section_creation_dispatcher::aligned_impl (std::uintptr_t const in) const {
            return pstore::aligned<bss_section> (in);
        }


        //*             _   _               _ _               _      _             *
        //*  ___ ___ __| |_(_)___ _ _    __| (_)____ __  __ _| |_ __| |_  ___ _ _  *
        //* (_-</ -_) _|  _| / _ \ ' \  / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_| *
        //* /__/\___\__|\__|_\___/_||_| \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_|   *
        //*                                       |_|                              *
        bss_section_dispatcher::~bss_section_dispatcher () noexcept = default;

    } // end namespace repo
} // end namespace pstore
