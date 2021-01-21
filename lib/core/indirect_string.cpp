//*  _           _ _               _         _        _              *
//* (_)_ __   __| (_)_ __ ___  ___| |_   ___| |_ _ __(_)_ __   __ _  *
//* | | '_ \ / _` | | '__/ _ \/ __| __| / __| __| '__| | '_ \ / _` | *
//* | | | | | (_| | | | |  __/ (__| |_  \__ \ |_| |  | | | | | (_| | *
//* |_|_| |_|\__,_|_|_|  \___|\___|\__| |___/\__|_|  |_|_| |_|\__, | *
//*                                                           |___/  *
//===- lib/core/indirect_string.cpp ---------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/core/indirect_string.hpp"

namespace pstore {

    //*  _         _ _            _        _       _            *
    //* (_)_ _  __| (_)_ _ ___ __| |_   __| |_ _ _(_)_ _  __ _  *
    //* | | ' \/ _` | | '_/ -_) _|  _| (_-<  _| '_| | ' \/ _` | *
    //* |_|_||_\__,_|_|_| \___\__|\__| /__/\__|_| |_|_||_\__, | *
    //*                                                  |___/  *

    // as string view
    // ~~~~~~~~~~~~~~
    raw_sstring_view
    indirect_string::as_string_view (gsl::not_null<shared_sstring_view *> const owner) const {
        if (is_pointer_) {
            return *str_;
        }
        if (address_ & in_heap_mask) {
            return *reinterpret_cast<sstring_view<char const *> const *> (address_ & ~in_heap_mask);
        }
        PSTORE_ASSERT (this->is_in_store ());
        *owner = serialize::read<shared_sstring_view> (
            serialize::archive::make_reader (db_, address{address_}));
        return {owner->data (), owner->length ()};
    }

    // operator<
    // ~~~~~~~~~
    bool indirect_string::operator< (indirect_string const & rhs) const {
        PSTORE_ASSERT (&db_ == &rhs.db_);
        shared_sstring_view lhs_owner;
        shared_sstring_view rhs_owner;
        return this->as_string_view (&lhs_owner) < rhs.as_string_view (&rhs_owner);
    }

    // operator==
    // ~~~~~~~~~~
    bool indirect_string::operator== (indirect_string const & rhs) const {
        PSTORE_ASSERT (&db_ == &rhs.db_);
        if (!is_pointer_ && !rhs.is_pointer_) {
            // We define that all strings in the database are unique. That means
            // that if both this and the rhs string are in the store then we can
            // simply compare their addresses.
            auto const equal = address_ == rhs.address_;
            PSTORE_ASSERT (this->equal_contents (rhs) == equal);
            return equal;
        }
        if (is_pointer_ && rhs.is_pointer_ && str_ == rhs.str_) {
            // Note that we can't immediately return false if str_ != rhs.str_
            // because two strings with different address may still have identical
            // contents.
            PSTORE_ASSERT (this->equal_contents (rhs));
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

    // write string and patch address
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    address
    indirect_string::write_body_and_patch_address (transaction_base & transaction,
                                                   raw_sstring_view const & str,
                                                   typed_address<address> const address_to_patch) {
        PSTORE_ASSERT (address_to_patch != typed_address<address>::null ());

        // Make sure the alignment of the string is 2 to ensure that the LSB is clear.
        constexpr auto aligned_to = std::size_t{1U << in_heap_mask};
        transaction.allocate (0, aligned_to);

        // Write the string body.
        auto const body_address =
            serialize::write (serialize::archive::make_writer (transaction), str);

        // Modify the in-store address field so that it points to the string body.
        auto const addr = transaction.getrw (address_to_patch);
        *addr = body_address;
        return body_address;
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

    // flush
    // ~~~~~
    void indirect_string_adder::flush (transaction_base & transaction) {
        for (auto const & v : views_) {
            PSTORE_ASSERT (v.second != typed_address<address>::null ());
            indirect_string::write_body_and_patch_address (transaction,
                                                           *std::get<0> (v), // string body
                                                           std::get<1> (v)   // address to patch
            );
        }
        views_.clear ();
    }

    //*  _        _                  __              _   _           *
    //* | |_  ___| |_ __  ___ _ _   / _|_  _ _ _  __| |_(_)___ _ _   *
    //* | ' \/ -_) | '_ \/ -_) '_| |  _| || | ' \/ _|  _| / _ \ ' \  *
    //* |_||_\___|_| .__/\___|_|   |_|  \_,_|_||_\__|\__|_\___/_||_| *
    //*            |_|                                               *
    // get sstring view
    // ~~~~~~~~~~~~~~~~
    auto get_sstring_view (database const & db, typed_address<indirect_string> const addr,
                           gsl::not_null<shared_sstring_view *> const owner) -> raw_sstring_view {
        return indirect_string::read (db, addr).as_db_string_view (owner);
    }

} // end namespace pstore
