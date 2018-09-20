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

namespace {
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

    /// The enum is mapped to the data type stored, and this type is mapped to a dispatcher
    /// subclass which provides a virtual interface to the underlying data.
    template <section_kind Kind>
    dispatcher_ptr make_dispatcher_for_kind (fragment const & f,
                                             pstore::gsl::not_null<dispatcher_buffer *> buffer) {
        using dispatcher_type =
            typename section_to_dispatcher<typename enum_to_section<Kind>::type>::type;
        static_assert (sizeof (dispatcher_type) <= sizeof (dispatcher_buffer),
                       "dispatcher buffer is too small");
        static_assert (alignof (dispatcher_type) <= alignof (dispatcher_buffer),
                       "dispatcher buffer is insufficiently aligned");
        return dispatcher_ptr (new (buffer) dispatcher_type (f.at<Kind> ()), nop_deleter{});
    }

    /// Constructs a dispatcher instance for the a specific section of the given fragment.
    ///
    /// \param f  The fragment.
    /// \param kind  The section that is being accessed. The fragment must hold a section of this
    /// kind. \param buffer A non-null pointer to a buffer into which the object will be
    /// constructed. In other words, on return the result will be a unique_ptr<> to an object inside
    /// this buffer. The lifetime of this buffer must be greater than that of the result pointer.
    dispatcher_ptr make_dispatcher (fragment const & f, section_kind kind,
                                    pstore::gsl::not_null<dispatcher_buffer *> buffer) {
        assert (f.has_section (kind));

#define X(k)                                                                                       \
    case section_kind::k: return make_dispatcher_for_kind<section_kind::k> (f, buffer);
        switch (kind) {
            PSTORE_MCREPO_SECTION_KINDS
        case section_kind::last: break;
        }
#undef X
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
        location, [&db](extent<fragment> const & x) { return db.getro (x); });
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
unsigned pstore::repo::section_align (fragment const & f, section_kind kind) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, kind, &buffer)->align ();
}

// section_size
// ~~~~~~~~~~~~~
std::size_t pstore::repo::section_size (fragment const & f, section_kind kind) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, kind, &buffer)->size ();
}

// section_ifixups
// ~~~~~~~~~~~~~~~
container<internal_fixup> pstore::repo::section_ifixups (fragment const & f,
                                                         section_kind kind) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, kind, &buffer)->ifixups ();
}

// section_xfixups
// ~~~~~~~~~~~~~~~
container<external_fixup> pstore::repo::section_xfixups (fragment const & f,
                                                         section_kind kind) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, kind, &buffer)->xfixups ();
}

// section_data
// ~~~~~~~~~~~~
container<std::uint8_t> pstore::repo::section_value (fragment const & f,
                                                     section_kind kind) {
    dispatcher_buffer buffer;
    return make_dispatcher (f, kind, &buffer)->data ();
}
