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

//*   __                             _        _      _         *
//*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_   __| |__ _| |_ __ _  *
//* |  _| '_/ _` / _` | '  \/ -_) ' \  _| / _` / _` |  _/ _` | *
//* |_| |_| \__,_\__, |_|_|_\___|_||_\__| \__,_\__,_|\__\__,_| *
//*              |___/                      				   *
//* *
// aligned_size_t
// ~~~~~~~~~~~~~~
std::size_t fragment_data::aligned_size_t (std::size_t a) const {
    static_assert (sizeof (std::uintptr_t) >= sizeof (std::size_t),
                   "sizeof uintptr_t should be at least sizeof size_t");
    return static_cast<std::size_t> (aligned (static_cast<std::uintptr_t> (a)));
}

// aligned_ptr
// ~~~~~~~~~~~
std::uint8_t * fragment_data::aligned_ptr (std::uint8_t * in) const {
    return reinterpret_cast<std::uint8_t *> (aligned (reinterpret_cast<std::uintptr_t> (in)));
}

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

//*             _   _               _      _         *
//*  ___ ___ __| |_(_)___ _ _    __| |__ _| |_ __ _  *
//* (_-</ -_) _|  _| / _ \ ' \  / _` / _` |  _/ _` | *
//* /__/\___\__|\__|_\___/_||_| \__,_\__,_|\__\__,_| *
//*                          						 *
// size_bytes
// ~~~~~~~~~~
std::size_t section_data::size_bytes () const {
    return section::size_bytes (section_->make_sources ());
}

// write
// ~~~~~
std::uint8_t * section_data::write (std::uint8_t * out) const {
    auto scn = new (out) section (section_->make_sources (), section_->align);
    return out + scn->size_bytes ();
}

// aligned
// ~~~~~~~
std::uintptr_t section_data::aligned (std::uintptr_t in) const {
    return pstore::aligned<section> (in);
}

//*     _                       _         _        *
//*  __| |___ _ __  ___ _ _  __| |___ _ _| |_ ___  *
//* / _` / -_) '_ \/ -_) ' \/ _` / -_) ' \  _(_-<  *
//* \__,_\___| .__/\___|_||_\__,_\___|_||_\__/__/  *
//*          |_|								   *
// size_bytes
// ~~~~~~~~~~
std::size_t dependents::size_bytes (std::uint64_t size) noexcept {
    if (size == 0) {
        return 0;
    }
    return sizeof (dependents) - sizeof (dependents::ticket_members_) +
           sizeof (dependents::ticket_members_[0]) * size;
}

// size_bytes
// ~~~~~~~~~~
/// Returns the number of bytes of storage required for the dependents.
std::size_t dependents::size_bytes () const noexcept {
    return size_bytes (this->size ());
}

// load
// ~~~~
std::shared_ptr<dependents const>
dependents::load (pstore::database const & db, pstore::typed_address<dependents> const dependent) {
    // First work out its size, then read the full-size of the object.
    std::shared_ptr<dependents const> ln = db.getro (dependent);
    return db.getro (dependent, dependents::size_bytes (ln->size ()));
}

//*     _                       _         _           _      _         *
//*  __| |___ _ __  ___ _ _  __| |___ _ _| |_ ___  __| |__ _| |_ __ _  *
//* / _` / -_) '_ \/ -_) ' \/ _` / -_) ' \  _(_-< / _` / _` |  _/ _` | *
//* \__,_\___| .__/\___|_||_\__,_\___|_||_\__/__/ \__,_\__,_|\__\__,_| *
//*          |_|                                                       *
// size_bytes
// ~~~~~~~~~~
std::size_t dependents_data::size_bytes () const {
    static_assert (sizeof (std::uint64_t) >= sizeof (std::uintptr_t),
                   "sizeof uint64_t should be at least sizeof uintptr_t");
    return dependents::size_bytes (static_cast<std::uint64_t> (end_ - begin_));
}

// write
// ~~~~~
std::uint8_t * dependents_data::write (std::uint8_t * out) const {
    if (begin_ == end_) {
        return out;
    }
    auto dependent = new (out) dependents (begin_, end_);
    return out + dependent->size_bytes ();
}

//*   __                             _    *
//*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_  *
//* |  _| '_/ _` / _` | '  \/ -_) ' \  _| *
//* |_| |_| \__,_\__, |_|_|_\___|_||_\__| *
//*              |___/                    *
// load
// ~~~~
std::shared_ptr<fragment const> fragment::load (pstore::database const & db,
                                                pstore::extent<fragment> const & location) {
    if (location.size >= sizeof (fragment)) {
        std::shared_ptr<fragment const> f = db.getro (location);
        auto ftype_indices = f->members ().get_indices ();
        if (!ftype_indices.empty () &&
            static_cast<fragment_type> (ftype_indices.back ()) < fragment_type::last &&
            f->size_bytes () == location.size) {
            return f;
        }
    }
    raise_error_code (std::make_error_code (error_code::bad_fragment_record));
}

// has_fragment
// ~~~~~~~~~~~~
bool fragment::has_fragment (fragment_type type) const noexcept {
    return arr_.has_index (static_cast<fragment::member_array::bitmap_type> (type));
}

// dependents
// ~~~~~~~~~~
dependents const * fragment::dependents () const noexcept {
    if (!has_fragment (fragment_type::dependent)) {
        return nullptr;
    }
    return &this->at<fragment_type::dependent> ();
}

dependents * fragment::dependents () noexcept {
    fragment const * cthis = this;
    return const_cast<class dependents *> (cthis->dependents ());
}

// num_sections
// ~~~~~~~~~~~~
std::size_t fragment::num_sections () const noexcept {
    return std::count_if (this->begin (), this->end (), is_section_type);
}

// size
// ~~~~
std::size_t fragment::size () const noexcept {
    return arr_.size ();
}

// size_bytes
// ~~~~~~~~~~
std::size_t fragment::size_bytes () const {
    if (arr_.size () == 0) {
        return sizeof (*this);
    }

    auto const last_offset = arr_.back ();
#define X(a)                                                                                       \
    case fragment_type::a:                                                                         \
        return last_offset +                                                                       \
               offset_to_instance<enum_to_section<fragment_type::a>::type> (last_offset)           \
                   .size_bytes ();
    switch (static_cast<fragment_type> (arr_.get_indices ().back ())) {
        PSTORE_REPO_SECTION_TYPES
        PSTORE_REPO_METADATA_TYPES
    case fragment_type::last: break;
    }
#undef X
    raise_error_code (std::make_error_code (error_code::bad_fragment_type));
}

namespace pstore {
    namespace repo {
        // section_align
        // ~~~~~~~~~~~~~
#define X(a)                                                                                       \
    case section_type::a: return Fragment.at<fragment_type::a> ().align ();

        std::uint8_t section_align (fragment const & Fragment, section_type type) {
            assert (Fragment.has_fragment (static_cast<fragment_type> (type)));
            switch (type) {
                PSTORE_REPO_SECTION_TYPES
            case section_type::last: break;
            }
            // Here to avoid a bogus warning from MSVC warning (claiming that not all control paths
            // return a value).
            raise_error_code (std::make_error_code (error_code::bad_fragment_type));
        }
#undef X

        // section_size
        // ~~~~~~~~~~~~~
#define X(a)                                                                                       \
    case section_type::a: return Fragment.at<fragment_type::a> ().data ().size ();
        std::size_t section_size (fragment const & Fragment, section_type type) {
            assert (Fragment.has_fragment (static_cast<fragment_type> (type)));
            switch (type) {
                PSTORE_REPO_SECTION_TYPES
            case section_type::last: break;
            }
            raise_error_code (std::make_error_code (error_code::bad_fragment_record));
        }
#undef X

        // section_ifixups
        // ~~~~~~~~~~~~~~~
#define X(a)                                                                                       \
    case section_type::a: return Fragment.at<fragment_type::a> ().ifixups ();

        section::container<internal_fixup> section_ifixups (fragment const & Fragment,
                                                            section_type type) {
            assert (Fragment.has_fragment (static_cast<fragment_type> (type)));
            switch (type) {
                PSTORE_REPO_SECTION_TYPES
            case section_type::last: break;
            }
            raise_error_code (std::make_error_code (error_code::bad_fragment_record));
        }
#undef X

        // section_xfixups
        // ~~~~~~~~~~~~~~~
#define X(a)                                                                                       \
    case section_type::a: return Fragment.at<fragment_type::a> ().xfixups ();

        section::container<external_fixup> section_xfixups (fragment const & Fragment,
                                                            section_type type) {
            assert (Fragment.has_fragment (static_cast<fragment_type> (type)));
            switch (type) {
                PSTORE_REPO_SECTION_TYPES
            case section_type::last: break;
            }
            raise_error_code (std::make_error_code (error_code::bad_fragment_record));
        }
#undef X

        // section_data
        // ~~~~~~~~~~~~
#define X(a)                                                                                       \
    case section_type::a: return Fragment.at<fragment_type::a> ().data ();

        section::container<std::uint8_t> section_value (fragment const & Fragment,
                                                        section_type type) {
            assert (Fragment.has_fragment (static_cast<fragment_type> (type)));
            switch (type) {
                PSTORE_REPO_SECTION_TYPES
            case section_type::last: break;
            }
            raise_error_code (std::make_error_code (error_code::bad_fragment_record));
        }
#undef X

    } // namespace repo
} // namespace pstore

// eof: lib/mcrepo/fragment.cpp
