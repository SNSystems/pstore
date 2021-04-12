//===- include/pstore/core/sstring_view_archive.hpp -------*- mode: C++ -*-===//
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
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file sstring_view_archive.hpp
/// \brief Defines serializer<> specializations for strings.

#ifndef PSTORE_CORE_SSTRING_VIEW_ARCHIVE_HPP
#define PSTORE_CORE_SSTRING_VIEW_ARCHIVE_HPP

#include "pstore/adt/sstring_view.hpp"
#include "pstore/core/db_archive.hpp"
#include "pstore/serialize/standard_types.hpp"
#include "pstore/serialize/types.hpp"
#include "pstore/support/varint.hpp"

namespace pstore {
    namespace serialize {

        /// \param db  The database containing the string to be read.
        /// \param addr  The pstore address of the string value.
        /// \param length  The number of bytes occupied by the string.
        /// \returns  A sstring_view<> object which provides a std::string_view-like view of the
        ///   store-based string.
        inline sstring_view<std::shared_ptr<char const>>
        read_string_view (database const & db, typed_address<char> addr, std::size_t const length) {
            return sstring_view<std::shared_ptr<char const>> (db.getro (addr, length), length);
        }

        /// \brief A serializer for sstring_view<std::shared_ptr<char const>>.
        template <>
        struct serializer<sstring_view<std::shared_ptr<char const>>> {
            using value_type = sstring_view<std::shared_ptr<char const>>;

            template <typename Archive>
            static auto write (Archive && archive, value_type const & str)
                -> archive_result_type<Archive> {
                return string_helper::write (std::forward<Archive> (archive), str);
            }

            /// \brief Reads an instance of `sstring_view` from an archiver.
            /// \param archive  The Archiver from which a string will be read.
            /// \param str  A reference to uninitialized memory that is suitable for a new string
            ///   instance.
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
                new (&str) value_type (read_string_view (
                    archive.get_db (), typed_address<char> (archive.get_address ()), length));
                archive.skip (length);
            }
        };

        template <>
        struct serializer<sstring_view<char const *>> {
            using value_type = sstring_view<char const *>;

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
        struct is_compatible<sstring_view<Pointer>, sstring_view<Pointer>> : std::true_type {};

        /// Any two sstring_view instances with the different Pointer type have the same serialized
        /// representation.
        template <typename Pointer1, typename Pointer2>
        struct is_compatible<sstring_view<Pointer1>, sstring_view<Pointer2>> : std::true_type {};

        /// sstring_view instances are serialized using the same format as std::string.
        template <typename Pointer1>
        struct is_compatible<sstring_view<Pointer1>, std::string> : std::true_type {};

        /// sstring_view instances are serialized using the same format as std::string.
        template <typename Pointer1>
        struct is_compatible<std::string, sstring_view<Pointer1>> : std::true_type {};


        /// \brief A serializer for sstring_view const. It delegates both read and write operations
        ///        to the sstring_view serializer.
        template <typename PointerType>
        struct serializer<sstring_view<PointerType> const> {
            using value_type = sstring_view<PointerType>;
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

    } // end namespace serialize
} // end namespace pstore

#endif // PSTORE_CORE_SSTRING_VIEW_ARCHIVE_HPP
