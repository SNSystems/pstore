//*  _           _ _               _         _        _              *
//* (_)_ __   __| (_)_ __ ___  ___| |_   ___| |_ _ __(_)_ __   __ _  *
//* | | '_ \ / _` | | '__/ _ \/ __| __| / __| __| '__| | '_ \ / _` | *
//* | | | | | (_| | | | |  __/ (__| |_  \__ \ |_| |  | | | | | (_| | *
//* |_|_| |_|\__,_|_|_|  \___|\___|\__| |___/\__|_|  |_|_| |_|\__, | *
//*                                                           |___/  *
//===- include/pstore/core/indirect_string.hpp ----------------------------===//
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

#ifndef PSTORE_CORE_INDIRECT_STRING_HPP
#define PSTORE_CORE_INDIRECT_STRING_HPP

#include <memory>

#include "pstore/core/sstring_view_archive.hpp"

namespace pstore {

    //*  _         _ _            _        _       _            *
    //* (_)_ _  __| (_)_ _ ___ __| |_   __| |_ _ _(_)_ _  __ _  *
    //* | | ' \/ _` | | '_/ -_) _|  _| (_-<  _| '_| | ' \/ _` | *
    //* |_|_||_\__,_|_|_| \___\__|\__| /__/\__|_| |_|_||_\__, | *
    //*                                                  |___/  *

    /// The string address can come in three forms:
    /// 1. An shared_sstring_view string that hasn't been added to the index yet. This is indicated
    /// when is_pointer_ is true. The str_ member points to the string view.
    /// 2. A database address which points to an in-memory shared_sstring_view. This happens when
    /// the string has been inserted, but the index has not yet been flushed. In this case,
    /// is_pointer_ is false and the LBS of address_ is set.
    /// 3. An address of a string in the store. In this case, is_pointer_ is false and the LSB of
    /// address_ is clear.
    ///
    /// The use of the LBS of the address field to distinguish between in-heap and in-store
    /// addresses means that the in-store string bodies must be 2-byte aligned.
    class indirect_string {
        friend struct serialize::serializer<indirect_string>;

    public:
        constexpr indirect_string (database const & db, address const addr) noexcept
                : db_{db}
                , is_pointer_{false}
                , address_{addr.absolute ()} {}
        indirect_string (database const & db,
                         gsl::not_null<raw_sstring_view const *> const str) noexcept
                : db_{db}
                , is_pointer_{true}
                , str_{str} {
            PSTORE_ASSERT ((reinterpret_cast<std::uintptr_t> (str.get ()) & in_heap_mask) == 0);
        }

        bool operator== (indirect_string const & rhs) const;
        bool operator!= (indirect_string const & rhs) const { return !operator== (rhs); }
        bool operator< (indirect_string const & rhs) const;

        raw_sstring_view as_string_view (gsl::not_null<shared_sstring_view *> owner) const;

        /// When it is known that the string body is a store address use this function to carry out
        /// additional checks that the address is reasonable.
        raw_sstring_view
        as_db_string_view (gsl::not_null<shared_sstring_view *> const owner) const {
            if (!is_in_store ()) {
                raise (error_code::bad_address);
            }
            return this->as_string_view (owner);
        }

        std::string to_string () const {
            shared_sstring_view owner;
            return this->as_string_view (&owner).to_string ();
        }

        /// \returns True is the pointee is in the store rather than on the heap.
        bool is_in_store () const noexcept { return !is_pointer_ && !(address_ & in_heap_mask); }

        /// Write the body of a string and updates the indirect pointer so that it points to that
        /// body.
        ///
        /// \param transaction  The transaction to which the string body is appended.
        /// \param str  The string to be written.
        /// \param address_to_patch  The in-store address of the indirect_string instance which will
        /// point to the string.
        /// \returns  The address at which the string body was written.
        static address write_body_and_patch_address (transaction_base & transaction,
                                                     raw_sstring_view const & str,
                                                     typed_address<address> address_to_patch);

        /// Reads an indirect string from the store.
        static indirect_string read (database const & db, typed_address<indirect_string> addr);

    private:
        static constexpr std::uint64_t in_heap_mask = 0x01;
        bool equal_contents (indirect_string const & rhs) const;

        database const & db_;
        // TODO: replace with std::variant<>...?
        /// True if the string address is in-heap and found in the str_ member. If false, the
        /// address_ field points to the string body in the store unless (address_ & in_heap_mask)
        /// in which case it is the heap address of the string.
        bool is_pointer_;
        union {
            std::uint64_t address_;        ///< The in-store/in-heap string address.
            raw_sstring_view const * str_; ///< The address of the in-heap string.
        };
    };

    template <typename OStream>
    OStream & operator<< (OStream & os, indirect_string const & ind_str) {
        shared_sstring_view owner;
        return os << ind_str.as_string_view (&owner);
    }


    namespace serialize {

        /// \brief A serializer for indirect_string.
        ///
        /// Note that this reads and writes an address: the body of the string must be read and
        /// written separately. For writing, see indirect_string::write_body_and_patch_address().
        template <>
        struct serializer<indirect_string> {
            using value_type = indirect_string;

            /// \brief Writes an instance of `indirect_string` to an archiver.
            ///
            /// \param archive  The Archiver to which the string will be written.
            /// \param value  The indirect_string instance to be serialized.
            /// \result  The address at which the data was written.
            /// \note This function only writes to a database.
            static auto write (archive::database_writer && archive, value_type const & value)
                -> archive_result_type<archive::database_writer> {
                return write_string_address (archive, value);
            }

            /// \brief Writes an instance of `indirect_string` to an archiver.
            ///
            /// \param archive  The Archiver to which the string will be written.
            /// \param value  The indirect_string instance to be serialized.
            /// \result  The address at which the data was written.
            /// \note This function only writes to a database.
            static auto write (archive::database_writer & archive, value_type const & value)
                -> archive_result_type<archive::database_writer> {
                return write_string_address (archive, value);
            }


            /// \brief Reads an instance of `indirect_string` from an archiver.
            ///
            /// \param archive  The Archiver from which a string will be read.
            /// \param value  A reference to uninitialized memory that is suitable for a new string
            /// instance.
            /// \note This function only reads from the database.
            static void read (archive::database_reader & archive, value_type & value);

            /// \brief Reads an instance of `indirect_string` from an archiver.
            ///
            /// \param archive  The Archiver from which a string will be read.
            /// \param value  A reference to uninitialized memory that is suitable for a new string
            /// instance.
            /// \note This function only reads from the database.
            static void read (archive::database_reader && archive, value_type & value);

        private:
            template <typename DBArchive>
            static auto write_string_address (DBArchive && archive, value_type const & value)
                -> archive_result_type<DBArchive>;

            template <typename DBArchive>
            static void read_string_address (DBArchive && archive, value_type & value);
        };

        // write_string_address
        // ~~~~~~~~~~~~~~~~~~~~
        template <typename DBArchive>
        auto serializer<indirect_string>::write_string_address (DBArchive && archive,
                                                                value_type const & value)
            -> archive_result_type<DBArchive> {

            // The body of an indirect string must be written separately by the caller.
            PSTORE_ASSERT (value.is_pointer_);
            constexpr auto mask = indirect_string::in_heap_mask;
            PSTORE_ASSERT (!(reinterpret_cast<std::uintptr_t> (value.str_) & mask));

            return archive.put (address{reinterpret_cast<std::uintptr_t> (value.str_) | mask});
        }

    } // end namespace serialize
} // end namespace pstore

namespace std {

    template <>
    struct hash<::pstore::indirect_string> {
        size_t operator() (::pstore::indirect_string const & str) const {
            ::pstore::shared_sstring_view owner;
            return std::hash<pstore::raw_sstring_view>{}(str.as_string_view (&owner));
        }
    };

} // namespace std

namespace pstore {

    //*  _         _ _            _        _       _                     _    _          *
    //* (_)_ _  __| (_)_ _ ___ __| |_   __| |_ _ _(_)_ _  __ _   __ _ __| |__| |___ _ _  *
    //* | | ' \/ _` | | '_/ -_) _|  _| (_-<  _| '_| | ' \/ _` | / _` / _` / _` / -_) '_| *
    //* |_|_||_\__,_|_|_| \___\__|\__| /__/\__|_| |_|_||_\__, | \__,_\__,_\__,_\___|_|   *
    //*                                                  |___/                           *

    /// indirect_string_adder is a helper class which handles the details of adding strings to the
    /// "indirect" index. To ensure that the string addresses cluster tightly, we must write in two
    /// phases. The first phase adds the entries to the index. A consequence of adding a string that
    /// is not already present in the index is that its indirect_string record is written
    /// immediately to the store. Once all of the strings have been added, we must then write their
    /// bodies (the string's actual character array, in other words). The bodies must be aligned
    /// according to indirect_string's requirements.
    class indirect_string_adder {
    public:
        indirect_string_adder () = default;
        /// \param expected_size  The anticipated number of strings being added to the index. The
        /// class records each of the added indirect strings in order that these addresses can be
        /// patched once the string bodies have been written.
        explicit indirect_string_adder (std::size_t expected_size);

        template <typename Index>
        std::pair<typename Index::iterator, bool> add (transaction_base & transaction,
                                                       std::shared_ptr<Index> const & index,
                                                       gsl::not_null<raw_sstring_view const *> str);

        void flush (transaction_base & transaction);

    private:
        std::vector<std::pair<raw_sstring_view const *, typed_address<address>>> views_;
    };

    // add
    // ~~~
    template <typename Index>
    std::pair<typename Index::iterator, bool>
    indirect_string_adder::add (transaction_base & transaction,
                                std::shared_ptr<Index> const & index,
                                gsl::not_null<raw_sstring_view const *> str) {

        // Inserting into the index immediately writes the indirect_string instance to the store if
        // the string isn't already in the set.
        auto res = index->insert (transaction, pstore::indirect_string{transaction.db (), str});
        if (res.second) {
            // Now the in-store addresses are pointing at the sstring_view instances on the heap.
            // If the string was written, we remember where it went.
            typename Index::iterator const & pos = res.first;
            views_.emplace_back (str, typed_address<address>::make (pos.get_address ()));
        }
        return res;
    }

    //*  _        _                  __              _   _           *
    //* | |_  ___| |_ __  ___ _ _   / _|_  _ _ _  __| |_(_)___ _ _   *
    //* | ' \/ -_) | '_ \/ -_) '_| |  _| || | ' \/ _|  _| / _ \ ' \  *
    //* |_||_\___|_| .__/\___|_|   |_|  \_,_|_||_\__|\__|_\___/_||_| *
    //*            |_|                                               *

    /// \param db  The database containing the string to be read.
    /// \param addr  The address of the indirect string pointer.
    /// \param owner  A pointer to the object which will own the memory containing the string.
    /// \result  A view of the requested string.
    auto get_sstring_view (database const & db, typed_address<indirect_string> const addr,
                           gsl::not_null<shared_sstring_view *> const owner) -> raw_sstring_view;

} // end namespace pstore

#endif // PSTORE_CORE_INDIRECT_STRING_HPP
