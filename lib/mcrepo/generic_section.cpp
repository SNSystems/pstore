//===- lib/mcrepo/generic_section.cpp -------------------------------------===//
//*                             _                      _   _              *
//*   __ _  ___ _ __   ___ _ __(_) ___   ___  ___  ___| |_(_) ___  _ __   *
//*  / _` |/ _ \ '_ \ / _ \ '__| |/ __| / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* | (_| |  __/ | | |  __/ |  | | (__  \__ \  __/ (__| |_| | (_) | | | | *
//*  \__, |\___|_| |_|\___|_|  |_|\___| |___/\___|\___|\__|_|\___/|_| |_| *
//*  |___/                                                                *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file generic_section.cpp
/// \brief Defines the generic section that is used for many fragment sections.
#include "pstore/mcrepo/generic_section.hpp"

namespace pstore {
    namespace repo {

        std::ostream & operator<< (std::ostream & os, internal_fixup const & ifx) {
            return os << "{ section:" << ifx.section
                      << ", type:" << static_cast<unsigned> (ifx.type) << ", offset:" << ifx.offset
                      << ", addend:" << ifx.addend << '}';
        }

        std::ostream & operator<< (std::ostream & os, binding const & b) {
            return os << (b == binding::weak ? "weak" : "strong");
        }

        std::ostream & operator<< (std::ostream & os, external_fixup const & xfx) {
            return os << "{ name:" << xfx.name << ", type:" << static_cast<unsigned> (xfx.type)
                      << ", binding:" << xfx.strength () << ", offset:" << xfx.offset
                      << ", addend:" << xfx.addend << '}';
        }

        //*             _   _                       _           _    *
        //*  ___ ___ __| |_(_)___ _ _    __ ___ _ _| |_ ___ _ _| |_  *
        //* (_-</ -_) _|  _| / _ \ ' \  / _/ _ \ ' \  _/ -_) ' \  _| *
        //* /__/\___\__|\__|_\___/_||_| \__\___/_||_\__\___|_||_\__| *
        //*                                                          *
        bool operator== (section_content const & lhs, section_content const & rhs) {
            return lhs.kind == rhs.kind && lhs.align == rhs.align && lhs.data == rhs.data &&
                   lhs.ifixups == rhs.ifixups && lhs.xfixups == rhs.xfixups;
        }

        bool operator!= (section_content const & lhs, section_content const & rhs) {
            return !operator== (lhs, rhs);
        }

        std::ostream & operator<< (std::ostream & os, section_content const & c) {
            os << "{ kind:" << c.kind << ", align:" << c.align << ", data:[";
            std::copy (std::begin (c.data), std::end (c.data),
                       std::ostream_iterator<unsigned> (os, ", "));
            os << "], ifixups:[";
            std::copy (std::begin (c.ifixups), std::end (c.ifixups),
                       std::ostream_iterator<internal_fixup> (os, ", "));
            os << "], xfixups:[";
            std::copy (std::begin (c.xfixups), std::end (c.xfixups),
                       std::ostream_iterator<external_fixup> (os, ", "));
            return os;
        }

        //*                        _                 _   _           *
        //*  __ _ ___ _ _  ___ _ _(_)__   ___ ___ __| |_(_)___ _ _   *
        //* / _` / -_) ' \/ -_) '_| / _| (_-</ -_) _|  _| / _ \ ' \  *
        //* \__, \___|_||_\___|_| |_\__| /__/\___\__|\__|_\___/_||_| *
        //* |___/                                                    *
        // size_bytes
        // ~~~~~~~~~~
        std::size_t generic_section::size_bytes (std::size_t const data_size,
                                                 std::size_t const num_ifixups,
                                                 std::size_t const num_xfixups) {
            auto result = sizeof (generic_section);
            result = generic_section::part_size_bytes<std::uint8_t> (result, data_size);
            result = generic_section::part_size_bytes<internal_fixup> (result, num_ifixups);
            result = generic_section::part_size_bytes<external_fixup> (result, num_xfixups);
            return result;
        }

        std::size_t generic_section::size_bytes () const {
            return generic_section::size_bytes (payload ().size (), ifixups ().size (),
                                                xfixups ().size ());
        }

        //*                  _   _               _ _               _      _             *
        //*  __ _ _ ___ __ _| |_(_)___ _ _    __| (_)____ __  __ _| |_ __| |_  ___ _ _  *
        //* / _| '_/ -_) _` |  _| / _ \ ' \  / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_| *
        //* \__|_| \___\__,_|\__|_\___/_||_| \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_|   *
        //*                                            |_|                              *

        std::size_t generic_section_creation_dispatcher::size_bytes () const {
            return generic_section::size_bytes (section_->make_sources ());
        }

        std::uint8_t * generic_section_creation_dispatcher::write (std::uint8_t * const out) const {
            PSTORE_ASSERT (this->aligned (out) == out);
            auto * const scn =
                new (out) generic_section (section_->make_sources (), section_->align);
            return out + scn->size_bytes ();
        }

        std::uintptr_t
        generic_section_creation_dispatcher::aligned_impl (std::uintptr_t const in) const {
            return pstore::aligned<generic_section> (in);
        }


        //*             _   _               _ _               _      _             *
        //*  ___ ___ __| |_(_)___ _ _    __| (_)____ __  __ _| |_ __| |_  ___ _ _  *
        //* (_-</ -_) _|  _| / _ \ ' \  / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_| *
        //* /__/\___\__|\__|_\___/_||_| \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_|   *
        //*                                       |_|                              *
        section_dispatcher::~section_dispatcher () noexcept {}


    } // end namespace repo
} // end namespace pstore
