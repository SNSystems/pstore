//*  _           _ _               _         _        _              *
//* (_)_ __   __| (_)_ __ ___  ___| |_   ___| |_ _ __(_)_ __   __ _  *
//* | | '_ \ / _` | | '__/ _ \/ __| __| / __| __| '__| | '_ \ / _` | *
//* | | | | | (_| | | | |  __/ (__| |_  \__ \ |_| |  | | | | | (_| | *
//* |_|_| |_|\__,_|_|_|  \___|\___|\__| |___/\__|_|  |_|_| |_|\__, | *
//*                                                           |___/  *
//===- lib/core/indirect_string.cpp ---------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
#include "pstore/core/indirect_string.hpp"

namespace pstore {

    raw_sstring_view
    indirect_string::as_string_view (gsl::not_null<shared_sstring_view *> const owner) const {
        if (is_pointer_) {
            return *str_;
        }
        if (address_ & in_heap_mask) {
            return *reinterpret_cast<sstring_view<char const *> const *> (address_ & ~in_heap_mask);
        }
        assert (this->is_in_store ());
        *owner = serialize::read<shared_sstring_view> (
            serialize::archive::make_reader (db_, address{address_}));
        return {owner->data (), owner->length ()};
    }

    // operator<
    // ~~~~~~~~~
    bool indirect_string::operator< (indirect_string const & rhs) const {
        assert (&db_ == &rhs.db_);
        shared_sstring_view lhs_owner;
        shared_sstring_view rhs_owner;
        return this->as_string_view (&lhs_owner) < rhs.as_string_view (&rhs_owner);
    }

    // operator==
    // ~~~~~~~~~~
    bool indirect_string::operator== (indirect_string const & rhs) const {
        assert (&db_ == &rhs.db_);
        if (!is_pointer_ && !rhs.is_pointer_) {
            // We define that all strings in the database are unique. That means
            // that if both this and the rhs string are in the store then we can
            // simply compare their addresses.
            auto const equal = address_ == rhs.address_;
            assert (this->equal_contents (rhs) == equal);
            return equal;
        }
        if (is_pointer_ && rhs.is_pointer_ && str_ == rhs.str_) {
            // Note that we can't immediately return false if str_ != rhs.str_
            // because two strings with different address may still have identical
            // contents.
            assert (this->equal_contents (rhs));
            return true;
        }
        return equal_contents (rhs);
    }

    // equal_contents
    // ~~~~~~~~~~~~~~
    bool indirect_string::equal_contents (indirect_string const & rhs) const {
        shared_sstring_view lhs_owner;
        shared_sstring_view rhs_owner;
        return this->as_string_view (&lhs_owner) == rhs.as_string_view (&rhs_owner);
    }


    namespace serialize {

        template <typename DBArchive>
        void serializer<indirect_string>::read_string_address (DBArchive && archive,
                                                               value_type & value) {
            database const & db = archive.get_db ();
            auto const addr = *db.getro (typed_address<address>::make (archive.get_address ()));
            new (&value) value_type (db, addr);
        }

        void serializer<indirect_string>::read (archive::database_reader & archive,
                                                value_type & value) {
            read_string_address (archive, value);
        }

        void serializer<indirect_string>::read (archive::database_reader && archive,
                                                value_type & value) {
            read_string_address (archive, value);
        }

    } // end namespace serialize


    //*  _         _ _            _        _       _                     _    _          *
    //* (_)_ _  __| (_)_ _ ___ __| |_   __| |_ _ _(_)_ _  __ _   __ _ __| |__| |___ _ _  *
    //* | | ' \/ _` | | '_/ -_) _|  _| (_-<  _| '_| | ' \/ _` | / _` / _` / _` / -_) '_| *
    //* |_|_||_\__,_|_|_| \___\__|\__| /__/\__|_| |_|_||_\__, | \__,_\__,_\__,_\___|_|   *
    //*                                                  |___/                           *
    // ctor
    // ~~~~
    indirect_string_adder::indirect_string_adder (std::size_t const expected_size) {
        views_.reserve (expected_size);
    }

    // read
    // ~~~~
    indirect_string indirect_string::read (database const & db,
                                           typed_address<indirect_string> const addr) {
        return serialize::read<indirect_string> (
            serialize::archive::make_reader (db, addr.to_address ()));
    }

    //*  _        _                  __              _   _           *
    //* | |_  ___| |_ __  ___ _ _   / _|_  _ _ _  __| |_(_)___ _ _   *
    //* | ' \/ -_) | '_ \/ -_) '_| |  _| || | ' \/ _|  _| / _ \ ' \  *
    //* |_||_\___|_| .__/\___|_|   |_|  \_,_|_||_\__|\__|_\___/_||_| *
    //*            |_|                                               *
    // get_sstring_view
    // ~~~~~~~~~~~~~~~~
    auto get_sstring_view (database const & db, typed_address<indirect_string> const addr,
                           gsl::not_null<shared_sstring_view *> const owner) -> raw_sstring_view {
        return indirect_string::read (db, addr).as_db_string_view (owner);
    }

} // end namespace pstore
