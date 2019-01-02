//*                             _                      _   _              *
//*   __ _  ___ _ __   ___ _ __(_) ___   ___  ___  ___| |_(_) ___  _ __   *
//*  / _` |/ _ \ '_ \ / _ \ '__| |/ __| / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* | (_| |  __/ | | |  __/ |  | | (__  \__ \  __/ (__| |_| | (_) | | | | *
//*  \__, |\___|_| |_|\___|_|  |_|\___| |___/\___|\___|\__|_|\___/|_| |_| *
//*  |___/                                                                *
//===- lib/mcrepo/generic_section.cpp -------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
#include "pstore/mcrepo/generic_section.hpp"

#include <cstring>

namespace pstore {
    namespace repo {

        //*                        _                 _   _           *
        //*  __ _ ___ _ _  ___ _ _(_)__   ___ ___ __| |_(_)___ _ _   *
        //* / _` / -_) ' \/ -_) '_| / _| (_-</ -_) _|  _| / _ \ ' \  *
        //* \__, \___|_||_\___|_| |_\__| /__/\___\__|\__|_\___/_||_| *
        //* |___/                                                    *
        // size_bytes
        // ~~~~~~~~~~
        std::size_t generic_section::size_bytes (std::size_t data_size, std::size_t num_ifixups,
                                                 std::size_t num_xfixups) {
            auto result = sizeof (generic_section);
            result = generic_section::part_size_bytes<std::uint8_t> (result, data_size);
            result = generic_section::part_size_bytes<internal_fixup> (result, num_ifixups);
            result = generic_section::part_size_bytes<external_fixup> (result, num_xfixups);
            return result;
        }

        std::size_t generic_section::size_bytes () const {
            return generic_section::size_bytes (data ().size (), ifixups ().size (),
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

        std::uint8_t * generic_section_creation_dispatcher::write (std::uint8_t * out) const {
            assert (this->aligned (out) == out);
            auto * const scn = new (out) generic_section (section_->make_sources (), section_->align);
            return out + scn->size_bytes ();
        }

        std::uintptr_t generic_section_creation_dispatcher::aligned_impl (std::uintptr_t in) const {
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
