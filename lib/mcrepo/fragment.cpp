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

#include <new>
#include "pstore/config/config.hpp"
#include "pstore/mcrepo/repo_error.hpp"
#include "pstore/support/gsl.hpp"

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


//*                  _   _               _ _               _      _                *
//*  __ _ _ ___ __ _| |_(_)___ _ _    __| (_)____ __  __ _| |_ __| |_  ___ _ _ ___ *
//* / _| '_/ -_) _` |  _| / _ \ ' \  / _` | (_-< '_ \/ _` |  _/ _| ' \/ -_) '_(_-< *
//* \__|_| \___\__,_|\__|_\___/_||_| \__,_|_/__/ .__/\__,_|\__\__|_||_\___|_| /__/ *
//*                                            |_|                                 *

//*******************
//* generic section *
//*******************
std::size_t generic_section_creation_dispatcher::size_bytes () const {
    return section::size_bytes (section_->make_sources ());
}

std::uint8_t * generic_section_creation_dispatcher::write (std::uint8_t * out) const {
    assert (this->aligned (out) == out);
    auto scn = new (out) section (section_->make_sources (), section_->align);
    return out + scn->size_bytes ();
}

std::uintptr_t generic_section_creation_dispatcher::aligned_impl (std::uintptr_t in) const {
    return pstore::aligned<section> (in);
}

//**************
//* dependents *
//**************
std::size_t dependents_creation_dispatcher::size_bytes () const {
    static_assert (sizeof (std::uint64_t) >= sizeof (std::uintptr_t),
                   "sizeof uint64_t should be at least sizeof uintptr_t");
    return dependents::size_bytes (static_cast<std::uint64_t> (end_ - begin_));
}

std::uint8_t * dependents_creation_dispatcher::write (std::uint8_t * out) const {
    assert (this->aligned (out) == out);
    if (begin_ == end_) {
        return out;
    }
    auto dependent = new (out) dependents (begin_, end_);
    return out + dependent->size_bytes ();
}

std::uintptr_t dependents_creation_dispatcher::aligned_impl (std::uintptr_t in) const {
    return pstore::aligned<dependents> (in);
}



namespace {

    /// This class is used to add virtual methods to a fragment's section. The section types
    /// themselves cannot be virtual because they're written to disk and wouldn't be portable
    /// between different C++ ABIs. The concrete classes derived from "dispatcher" wrap the real
    /// section data types and forward to calls directly to them.
    class dispatcher {
    public:
        virtual ~dispatcher () noexcept = default;

        virtual std::size_t size_bytes () const = 0;
        virtual unsigned align () const = 0;
        virtual std::size_t size () const = 0;
        virtual section::container<internal_fixup> ifixups () const = 0;
        virtual section::container<external_fixup> xfixups () const = 0;
        virtual section::container<std::uint8_t> data () const = 0;
    };

    class section_dispatcher final : public dispatcher {
    public:
        explicit section_dispatcher (section const & s) noexcept
                : s_{s} {}

        std::size_t size_bytes () const final { return s_.size_bytes (); }
        unsigned align () const final { return s_.align (); }
        std::size_t size () const final { return s_.data ().size (); }
        section::container<internal_fixup> ifixups () const final { return s_.ifixups (); }
        section::container<external_fixup> xfixups () const final { return s_.xfixups (); }
        section::container<std::uint8_t> data () const final { return s_.data (); }

    private:
        section const & s_;
    };

    class dependents_dispatcher final : public dispatcher {
    public:
        explicit dependents_dispatcher (dependents const & d) noexcept
                : d_{d} {}

        std::size_t size_bytes () const final { return d_.size_bytes (); }
        unsigned align () const final { error (); }
        std::size_t size () const final { error (); }
        section::container<internal_fixup> ifixups () const final { error (); }
        section::container<external_fixup> xfixups () const final { error (); }
        section::container<std::uint8_t> data () const final { error (); }

    private:
        PSTORE_NO_RETURN void error () const {
            pstore::raise_error_code (
                std::make_error_code (pstore::repo::error_code::bad_fragment_type));
        }
        dependents const & d_;
    };

    /// Maps from the type of data that is associated with a fragment's section to a "dispatcher"
    /// subclass which provides a generic interface to the behavior of these sections.
    template <typename T>
    struct section_to_dispatcher {};
    template <>
    struct section_to_dispatcher<section> {
        using type = section_dispatcher;
    };
    template <>
    struct section_to_dispatcher<dependents> {
        using type = dependents_dispatcher;
    };

    /// The size and alignment necessary for a buffer into which any of the dispatcher subclasses
    /// can be successfully constructed. This must list all of the dispatcher subclasses.
    using dispatcher_characteristics =
        pstore::characteristics<section_dispatcher, dependents_dispatcher>;

    /// A type which is suitable for holding an instance of any of the dispatcher subclasses.
    using dispatcher_buffer = std::aligned_storage<dispatcher_characteristics::size,
                                                   dispatcher_characteristics::align>::type;

    struct nop_deleter {
        void operator() (dispatcher * const) const noexcept {}
    };
    using dispatcher_ptr = std::unique_ptr<dispatcher, nop_deleter>;

    /// Constructs a dispatcher instance for the a specific section of the given fragment.
    /// \param buffer A non-null pointer to a buffer into which the object will be constructed. In
    /// other words, on return the result will be a unique_ptr<> to an object inside this buffer.
    /// The lifetime of this buffer must be greater than that of the result pointer.
    dispatcher_ptr make_dispatcher (fragment const & f, fragment_type type,
                                    pstore::gsl::not_null<dispatcher_buffer *> buffer) {
        assert (f.has_fragment (static_cast<fragment_type> (type)));

        // This ugly marco is instantiated for each of the various types in the section_type enum.
        // The enum is mapped to the data type stored, and this type is mapped to a dispatcher
        // subclass which provides a virtual interface to the underlying data.
#define X(a)                                                                                       \
    case fragment_type::a: {                                                                       \
        using dispatcher_type =                                                                    \
            section_to_dispatcher<enum_to_section<fragment_type::a>::type>::type;                  \
        static_assert (sizeof (dispatcher_type) <= sizeof (dispatcher_buffer),                     \
                       "dispatcher buffer is too small");                                          \
        static_assert (alignof (dispatcher_type) <= alignof (dispatcher_buffer),                   \
                       "dispatcher buffer is insufficiently aligned");                             \
        return dispatcher_ptr (new (buffer) dispatcher_type (f.at<fragment_type::a> ()),           \
                               nop_deleter{});                                                     \
    }
        switch (type) {
            PSTORE_REPO_SECTION_TYPES
            PSTORE_REPO_METADATA_TYPES
        case fragment_type::last: break;
        }
#undef X
        // Here to avoid a bogus warning from MSVC warning (claiming that not all control paths
        // return a value).
        pstore::raise_error_code (
            std::make_error_code (pstore::repo::error_code::bad_fragment_type));
    }

} // end anonymous namespace


//*   __                             _    *
//*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_  *
//* |  _| '_/ _` / _` | '  \/ -_) ' \  _| *
//* |_| |_| \__,_\__, |_|_|_\___|_||_\__| *
//*              |___/                    *
// load
// ~~~~
std::shared_ptr<fragment const> fragment::load (pstore::database const & db,
                                                pstore::extent<fragment> const & location) {
    return load_impl<std::shared_ptr<fragment const>> (
        location, [&db](pstore::extent<fragment> const & x) { return db.getro (x); });
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
    return static_cast<std::size_t> (std::count_if (this->begin (), this->end (), is_section_type));
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
    auto const last_section_type = static_cast<fragment_type> (arr_.get_indices ().back ());
    dispatcher_buffer buffer;
    auto const last_section_size =
        make_dispatcher (*this, last_section_type, &buffer)->size_bytes ();
    return last_offset + last_section_size;
}


// section_align
// ~~~~~~~~~~~~~
unsigned pstore::repo::section_align (fragment const & f, section_type type) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, static_cast<fragment_type> (type), &buffer)->align ();
}

// section_size
// ~~~~~~~~~~~~~
std::size_t pstore::repo::section_size (fragment const & f, section_type type) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, static_cast<fragment_type> (type), &buffer)->size ();
}

// section_ifixups
// ~~~~~~~~~~~~~~~
section::container<internal_fixup> pstore::repo::section_ifixups (fragment const & f,
                                                                  section_type type) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, static_cast<fragment_type> (type), &buffer)->ifixups ();
}

// section_xfixups
// ~~~~~~~~~~~~~~~
section::container<external_fixup> pstore::repo::section_xfixups (fragment const & f,
                                                                  section_type type) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, static_cast<fragment_type> (type), &buffer)->xfixups ();
}

// section_data
// ~~~~~~~~~~~~
section::container<std::uint8_t> pstore::repo::section_value (fragment const & f,
                                                              section_type type) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, static_cast<fragment_type> (type), &buffer)->data ();
}
