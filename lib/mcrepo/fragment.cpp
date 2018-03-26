//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===- lib/mcrepo/fragment.cpp --------------------------------------------===//
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
#include "pstore/mcrepo/fragment.hpp"

#include "pstore/config/config.hpp"
#include "pstore/mcrepo/repo_error.hpp"

using namespace pstore::repo;

//*             _   _           *
//*  ___ ___ __| |_(_)___ _ _   *
//* (_-</ -_) _|  _| / _ \ ' \  *
//* /__/\___\__|\__|_\___/_||_| *
//*                             *
// size_bytes
// ~~~~~~~~~~
std::size_t section::size_bytes (std::size_t data_size, std::size_t num_ifixups,
                                 std::size_t num_xfixups) {
    auto result = sizeof (section);
    result = section::part_size_bytes<std::uint8_t> (result, data_size);
    result = section::part_size_bytes<internal_fixup> (result, num_ifixups);
    result = section::part_size_bytes<external_fixup> (result, num_xfixups);
    return result;
}

std::size_t section::size_bytes () const {
    return section::size_bytes (data ().size (), ifixups ().size (), xfixups ().size ());
}

// three_byte_integer::get
// ~~~~~~~~~~~~~~~~~~~~~~~
std::uint32_t section::three_byte_integer::get (std::uint8_t const * src) noexcept {
    number result;
#if PSTORE_IS_BIG_ENDIAN
    result.bytes[0] = 0;
    std::memcpy (&result[1], src, 3);
#else
    std::memcpy (&result, src, 3);
    result.bytes[3] = 0;
#endif
    return result.value;
}

// three_byte_integer::set
// ~~~~~~~~~~~~~~~~~~~~~~~
void section::three_byte_integer::set (std::uint8_t * out, std::uint32_t v) noexcept {
    constexpr auto out_bytes = std::size_t{3};
    number num;
    num.value = v;

#if PSTORE_IS_BIG_ENDIAN
    constexpr auto first_byte = sizeof (std::uint32_t) - num_bytes;
#else
    constexpr auto first_byte = 0U;
#endif
    std::memcpy (out, &num.bytes[first_byte], out_bytes);
}

//*   __                             _    *
//*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_  *
//* |  _| '_/ _` / _` | '  \/ -_) ' \  _| *
//* |_| |_| \__,_\__, |_|_|_\___|_||_\__| *
//*              |___/                    *
// load
// ~~~~
std::shared_ptr<fragment const> fragment::load (pstore::database const & db,
                                                pstore::extent const & location) {
    if (location.size >= sizeof (fragment)) {
        auto f = std::static_pointer_cast<fragment const> (db.getro (location));
        if (f->size_bytes () == location.size) {
            return f;
        }
    }
    raise_error_code (std::make_error_code (error_code::bad_fragment_record));
}

// operator[]
// ~~~~~~~~~~
section const & fragment::operator[] (section_type key) const {
    auto const offset = arr_[static_cast<std::size_t> (key)];
    auto const ptr = reinterpret_cast<std::uint8_t const *> (this) + offset;
    assert (reinterpret_cast<std::uintptr_t> (ptr) % alignof (section) == 0);
    return *reinterpret_cast<section const *> (ptr);
}

// offset_to_section
// ~~~~~~~~~~~~~~~~~
section const & fragment::offset_to_section (std::uint64_t offset) const {
    auto const ptr = reinterpret_cast<std::uint8_t const *> (this) + offset;
    if (reinterpret_cast<std::uintptr_t> (ptr) % alignof (section) != 0) {
        raise_error_code (std::make_error_code (error_code::bad_fragment_record));
    }
    return *reinterpret_cast<section const *> (ptr);
}

// size_bytes
// ~~~~~~~~~~
std::size_t fragment::size_bytes () const {
    if (arr_.size () == 0) {
        return sizeof (*this);
    }
    auto const last_section_offset = arr_.back ();
    auto const & section = offset_to_section (last_section_offset);
    return last_section_offset + section.size_bytes ();
}

// eof: lib/mcrepo/fragment.cpp
