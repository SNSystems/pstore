//===- lib/mcrepo/debug_line_section.cpp ----------------------------------===//
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
#include "pstore/mcrepo/debug_line_section.hpp"

namespace pstore {
    namespace repo {

        std::size_t debug_line_section_creation_dispatcher::size_bytes () const {
            return debug_line_section::size_bytes (section_->make_sources ());
        }

        std::uint8_t *
        debug_line_section_creation_dispatcher::write (std::uint8_t * const out) const {
            PSTORE_ASSERT (this->aligned (out) == out);
            auto * const scn = new (out) debug_line_section (
                header_digest_, header_, section_->make_sources (), section_->align);
            return out + scn->size_bytes ();
        }

        std::uintptr_t
        debug_line_section_creation_dispatcher::aligned_impl (std::uintptr_t const in) const {
            return pstore::aligned<debug_line_section> (in);
        }

        debug_line_dispatcher::~debug_line_dispatcher () noexcept = default;

    } // end namespace repo
} // end namespace pstore
