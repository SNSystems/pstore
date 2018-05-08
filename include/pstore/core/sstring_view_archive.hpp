//*          _        _                     _                *
//*  ___ ___| |_ _ __(_)_ __   __ _  __   _(_) _____      __ *
//* / __/ __| __| '__| | '_ \ / _` | \ \ / / |/ _ \ \ /\ / / *
//* \__ \__ \ |_| |  | | | | | (_| |  \ V /| |  __/\ V  V /  *
//* |___/___/\__|_|  |_|_| |_|\__, |   \_/ |_|\___| \_/\_/   *
//*                           |___/                          *
//*                 _     _            *
//*   __ _ _ __ ___| |__ (_)_   _____  *
//*  / _` | '__/ __| '_ \| \ \ / / _ \ *
//* | (_| | | | (__| | | | |\ V /  __/ *
//*  \__,_|_|  \___|_| |_|_| \_/ \___| *
//*                                    *
//===- include/pstore/core/sstring_view_archive.hpp -----------------------===//
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
/// \file sstring_view_archive.hpp

#ifndef PSTORE_SSTRING_VIEW_ARCHIVE_HPP
#define PSTORE_SSTRING_VIEW_ARCHIVE_HPP

#include "pstore/core/db_archive.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/core/varint.hpp"
#include "pstore/serialize/standard_types.hpp"
#include "pstore/serialize/types.hpp"
#include "pstore/support/sstring_view.hpp"

namespace pstore {
    namespace serialize {

        /// \brief A serializer for sstring_view<std::shared_ptr<char const>>.
        template <>
        struct serializer<::pstore::sstring_view<std::shared_ptr<char const>>> {
            using value_type = ::pstore::sstring_view<std::shared_ptr<char const>>;

            template <typename Archive>
            static auto write (Archive && archive, value_type const & str)
                -> archive_result_type<Archive> {
                return string_helper::write (std::forward<Archive> (archive), str);
            }

            /// \brief Reads an instance of `sstring_view` from an archiver.
            /// \param archive  The Archiver from which a string will be read.
            /// \param str  A reference to uninitialized memory that is suitable for a new string
            /// instance.
            /// \note This function only reads from the database.
            static void read (archive::database_reader && archive, value_type & str) {
                readsv (archive, str);
            }
            static void read (archive::database_reader & archive, value_type & str) {
                readsv (archive, str);
            }

        private:
            template <typename DBReader>
            static void readsv (DBReader && archive, value_type & str) {
                std::size_t const length =
                    string_helper::read_length (std::forward<DBReader> (archive));
                new (&str) value_type (
                    archive.get_db ().template getro<char> (archive.get_address (), length),
                    length);
                archive.skip (length);
            }
        };

        template <>
        struct serializer<::pstore::sstring_view<char const *>> {
            using value_type = ::pstore::sstring_view<char const *>;

            template <typename Archive>
            static auto write (Archive && archive, value_type const & str)
                -> archive_result_type<Archive> {
                return string_helper::write (archive, str);
            }
            // note that there's no read() implementation.
        };

        /// Any two sstring_view instances with the same Pointer type have the same serialized
        /// representation.
        template <typename Pointer>
        struct is_compatible<::pstore::sstring_view<Pointer>, ::pstore::sstring_view<Pointer>>
                : public std::true_type {};

        /// Any two sstring_view instances with the different Pointer type have the same serialized
        /// representation.
        template <typename Pointer1, typename Pointer2>
        struct is_compatible<::pstore::sstring_view<Pointer1>, ::pstore::sstring_view<Pointer2>>
                : public std::true_type {};

        /// sstring_view instances are serialized using the same format as std::string.
        template <typename Pointer1>
        struct is_compatible<::pstore::sstring_view<Pointer1>, std::string>
                : public std::true_type {};

        /// sstring_view instances are serialized using the same format as std::string.
        template <typename Pointer1>
        struct is_compatible<std::string, ::pstore::sstring_view<Pointer1>>
                : public std::true_type {};


        /// \brief A serializer for sstring_view const. It delegates both read and write operations
        ///        to the sstring_view serializer.
        template <typename PointerType>
        struct serializer<::pstore::sstring_view<PointerType> const> {
            using value_type = ::pstore::sstring_view<PointerType>;
            template <typename Archive>
            static auto write (Archive && archive, value_type const & str)
                -> archive_result_type<Archive> {
                return serializer::write (std::forward<Archive> (archive), str);
            }
            template <typename Archive>
            static void read (Archive && archive, value_type & str) {
                serialize::read_uninit (std::forward<Archive> (archive), str);
            }
        };

    } // namespace serialize
} // namespace pstore

#endif // PSTORE_SSTRING_VIEW_ARCHIVE_HPP
// eof: include/pstore/core/sstring_view_archive.hpp
