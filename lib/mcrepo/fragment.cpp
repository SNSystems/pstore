//===- lib/mcrepo/fragment.cpp --------------------------------------------===//
//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/mcrepo/fragment.hpp"

#include <new>

#include "pstore/config/config.hpp"
#include "pstore/mcrepo/repo_error.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/max.hpp"

using namespace pstore::repo;

namespace {

    /// The size and alignment necessary for a buffer into which any of the dispatcher subclasses
    /// can be successfully constructed. This must list all of the dispatcher subclasses.
    using dispatcher_characteristics =
        pstore::characteristics<section_dispatcher, bss_section_dispatcher, debug_line_dispatcher,
                                linked_definitions_dispatcher>;

    /// A type which is suitable for holding an instance of any of the dispatcher subclasses.
    using dispatcher_buffer = std::aligned_storage<dispatcher_characteristics::size,
                                                   dispatcher_characteristics::align>::type;

    struct nop_deleter {
        void operator() (dispatcher * const) const noexcept {}
    };
    using dispatcher_ptr = std::unique_ptr<dispatcher, nop_deleter>;

    /// The enum is mapped to the data type stored, and this type is mapped to a dispatcher
    /// subclass which provides a virtual interface to the underlying data.
    template <section_kind Kind>
    dispatcher_ptr
    make_dispatcher_for_kind (fragment const & f,
                              pstore::gsl::not_null<dispatcher_buffer *> const buffer) {
        using dispatcher_type =
            typename section_to_dispatcher<typename enum_to_section<Kind>::type>::type;
        static_assert (sizeof (dispatcher_type) <= sizeof (dispatcher_buffer),
                       "dispatcher buffer is too small");
        static_assert (alignof (dispatcher_type) <= alignof (dispatcher_buffer),
                       "dispatcher buffer is insufficiently aligned");
        return dispatcher_ptr (new (buffer) dispatcher_type (f.at<Kind> ()), nop_deleter{});
    }

    /// Constructs a dispatcher instance for a specific section of the given fragment.
    ///
    /// \param f  The fragment.
    /// \param kind  The section that is being accessed. The fragment must hold a section of this
    /// kind.
    /// \param buffer A non-null pointer to a buffer into which the object will be
    /// constructed. In other words, on return the result will be a unique_ptr<> to an object inside
    /// this buffer. The lifetime of this buffer must be greater than that of the result pointer.
    dispatcher_ptr make_dispatcher (fragment const & f, section_kind const kind,
                                    pstore::gsl::not_null<dispatcher_buffer *> const buffer) {
        PSTORE_ASSERT (f.has_section (kind));

#define X(k)                                                                                       \
    case section_kind::k: return make_dispatcher_for_kind<section_kind::k> (f, buffer);
        switch (kind) {
            PSTORE_MCREPO_SECTION_KINDS
        case section_kind::last: break;
        }
#undef X
        pstore::raise_error_code (make_error_code (pstore::repo::error_code::bad_fragment_type));
    }

} // end anonymous namespace


//*   __                             _    *
//*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_  *
//* |  _| '_/ _` / _` | '  \/ -_) ' \  _| *
//* |_| |_| \__,_\__, |_|_|_\___|_||_\__| *
//*              |___/                    *

constexpr std::array<char, 8> fragment::fragment_signature_;

// load
// ~~~~
std::shared_ptr<fragment const> fragment::load (pstore::database const & db,
                                                pstore::extent<fragment> const & location) {
    return load_impl<std::shared_ptr<fragment const>> (
        location, [&db] (extent<fragment> const & x) { return db.getro (x); });
}

// section_offset_is_valid [static]
// ~~~~~~~~~~~~~~~~~~~~~~~
template <section_kind Key, typename InstanceType>
std::uint64_t
fragment::section_offset_is_valid (fragment const & f, pstore::extent<fragment> const & fext,
                                   std::uint64_t const min_offset, std::uint64_t const offset,
                                   std::size_t const size) {
    PSTORE_STATIC_ASSERT (alignof (fragment) >= alignof (InstanceType));
    return ((fext.addr.absolute () + offset) % alignof (InstanceType) != 0 ||
            size < sizeof (InstanceType) || offset < min_offset || size > fext.size ||
            offset > fext.size - size ||
            f.offset_to_instance<InstanceType> (offset).size_bytes () > size)
               ? 0
               : offset + size;
}

// fragment_appears_valid [static]
// ~~~~~~~~~~~~~~~~~~~~~~
bool fragment::fragment_appears_valid (fragment const & f, pstore::extent<fragment> const & fext) {
#if PSTORE_SIGNATURE_CHECKS_ENABLED
    if (f.signature_ != fragment_signature_) {
        return false;
    }
#endif // PSTORE_SIGNATURE_CHECKS_ENABLED

    auto const indices = f.arr_.get_indices ();
    if (indices.empty () || indices.back () >= section_kind::last) {
        return false;
    }

    std::uint64_t offset = sizeof (fragment);
    for (auto index_it = std::begin (indices), index_end = std::end (indices);
         index_it != index_end; ++index_it) {
        section_kind const index = *index_it;

        auto const this_offset = f.arr_[index];
        auto const next_offset = (index == indices.back ()) ? fext.size : f.arr_[*(index_it + 1)];
        if (this_offset < offset || next_offset <= this_offset) {
            return false;
        }

        section_kind const kind = static_cast<section_kind> (index);
        PSTORE_ASSERT (f.has_section (kind));
#define X(k)                                                                                       \
    case section_kind::k:                                                                          \
        offset = fragment::section_offset_is_valid<section_kind::k> (f, fext, offset, this_offset, \
                                                                     next_offset - this_offset);   \
        break;

        switch (kind) {
            PSTORE_MCREPO_SECTION_KINDS
        case repo::section_kind::last: PSTORE_ASSERT (false); break;
        }
#undef X
        if (offset == 0) {
            return false;
        }
    }

    return true;
}

// size_bytes
// ~~~~~~~~~~
std::size_t fragment::size_bytes () const {
    if (arr_.size () == 0) {
        return sizeof (*this);
    }
    auto const last_offset = arr_.back ();
    auto const last_section_kind = static_cast<section_kind> (arr_.get_indices ().back ());
    dispatcher_buffer buffer;
    auto const last_section_size =
        make_dispatcher (*this, last_section_kind, &buffer)->size_bytes ();
    return last_offset + last_section_size;
}


// section_align
// ~~~~~~~~~~~~~
unsigned pstore::repo::section_align (fragment const & f, section_kind const kind) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, kind, &buffer)->align ();
}

// section_size
// ~~~~~~~~~~~~~
std::size_t pstore::repo::section_size (fragment const & f, section_kind const kind) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, kind, &buffer)->size ();
}

// section_ifixups
// ~~~~~~~~~~~~~~~
container<internal_fixup> pstore::repo::section_ifixups (fragment const & f,
                                                         section_kind const kind) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, kind, &buffer)->ifixups ();
}

// section_xfixups
// ~~~~~~~~~~~~~~~
container<external_fixup> pstore::repo::section_xfixups (fragment const & f,
                                                         section_kind const kind) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, kind, &buffer)->xfixups ();
}

// section_data
// ~~~~~~~~~~~~
container<std::uint8_t> pstore::repo::section_value (fragment const & f, section_kind const kind) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, kind, &buffer)->payload ();
}
