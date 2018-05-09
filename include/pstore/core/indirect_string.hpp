//*  _           _ _               _         _        _              *
//* (_)_ __   __| (_)_ __ ___  ___| |_   ___| |_ _ __(_)_ __   __ _  *
//* | | '_ \ / _` | | '__/ _ \/ __| __| / __| __| '__| | '_ \ / _` | *
//* | | | | | (_| | | | |  __/ (__| |_  \__ \ |_| |  | | | | | (_| | *
//* |_|_| |_|\__,_|_|_|  \___|\___|\__| |___/\__|_|  |_|_| |_|\__, | *
//*                                                           |___/  *
//===- include/pstore/core/indirect_string.hpp ----------------------------===//
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

#ifndef PSTORE_CORE_INDIRECT_STRING_HPP
#define PSTORE_CORE_INDIRECT_STRING_HPP (1)

#include <memory>

#include "pstore/core/db_archive.hpp"
#include "pstore/core/sstring_view_archive.hpp"

namespace pstore {

    using shared_sstring_view = sstring_view<std::shared_ptr<char const>>;

    // replace with std::variant<>...?
    /// The string address can come in three forms:
    /// 1. An shared_sstring_view string that hasn't been added to the index yet. This is indicated
    /// when is_address_ is false.
    /// 2. A database address which points to an in-memory shared_sstring_view. This happens when
    /// the string has been inserted, but the index has not yet been flushed.
    /// 3. An address of a string in the store.
    class indirect_string {
    public:
        explicit indirect_string (database & db, address addr)
                : db_{db}
                , is_pointer_{false}
                , address_{addr.absolute ()} {}
        explicit indirect_string (database & db, shared_sstring_view const * str)
                : db_{db}
                , is_pointer_{true}
                , str_{str} {}

        shared_sstring_view as_string_view () const;

        database & db_;
        bool is_pointer_;
        union {
            std::uint64_t address_;           // the in-pstore string address.
            shared_sstring_view const * str_; // the address of the in-memory string.
        };
    };

    bool operator== (indirect_string const & lhs, indirect_string const & rhs);
    inline bool operator!= (indirect_string const & lhs, indirect_string const & rhs) {
        return !operator== (lhs, rhs);
    }

    namespace serialize {

        /// \brief A serializer for string_address.
        template <>
        struct serializer<indirect_string> {
            using value_type = indirect_string;

            template <typename Transaction>
            static auto write (archive::database_writer<Transaction> && archive,
                               value_type const & value)
                -> archive_result_type<archive::database_writer<Transaction>> {

                // The body of an indirect string must be written separately by the caller.
                assert (value.is_pointer_);
                assert (!(reinterpret_cast<std::uintptr_t> (value.str_) & 0x01));
                auto const addr = static_cast<std::uint64_t> (
                    reinterpret_cast<std::uintptr_t> (value.str_) | 0x01);
                return archive.put (addr);
            }

            /// \brief Reads an instance of `string_address` from an archiver.
            ///
            /// \param archive  The Archiver from which a string will be read.
            /// \param value  A reference to uninitialized memory that is suitable for a new string
            /// instance.
            /// \note This function only reads from the database.
            static void read (archive::database_reader & archive, value_type & value) {
                read_string_address (archive, value);
            }

            static void read (archive::database_reader && archive, value_type & value) {
                read_string_address (std::forward<archive::database_reader> (archive), value);
            }

        private:
            template <typename DBArchive>
            static void read_string_address (DBArchive && archive, value_type & value) {
                database & db = archive.get_db ();
                address const addr = *db.getro<address> (archive.get_address ());
                new (&value) value_type (db, addr);
            }
        };

    } // end namespace serialize
} // end namespace pstore

#endif // PSTORE_CORE_INDIRECT_STRING_HPP
// eof: include/pstore/core/indirect_string.hpp
