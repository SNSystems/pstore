//*          _        _                     _                *
//*  ___ ___| |_ _ __(_)_ __   __ _  __   _(_) _____      __ *
//* / __/ __| __| '__| | '_ \ / _` | \ \ / / |/ _ \ \ /\ / / *
//* \__ \__ \ |_| |  | | | | | (_| |  \ V /| |  __/\ V  V /  *
//* |___/___/\__|_|  |_|_| |_|\__, |   \_/ |_|\___| \_/\_/   *
//*                           |___/                          *
//===- include/pstore/sstring_view_archive.hpp ----------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_SSTRING_VIEW_ARCHIVE_HPP
#define PSTORE_SSTRING_VIEW_ARCHIVE_HPP

#include "pstore/serialize/types.hpp"
#include "pstore/sstring_view.hpp"
#include "pstore/varint.hpp"

///@{
/// \name Reading and writing standard types
/// A collection of convenience methods which each know how to serialize the types defined by the
/// standard library (string, vector, set, etc.)
namespace pstore {
    namespace serialize {
        namespace archive {
            class database_reader;
        }

        /// \brief A serializer for std::string.
        template <>
        struct serializer<::pstore::sstring_view> {
            using value_type = ::pstore::sstring_view;

            /// \brief Writes an instance of `std::string` to an archive.
            ///
            /// Writes a variable length value follow by a sequence of characters. The
            /// length uses the format defined by varint::encode() but we ensure that at
            /// least two bytes are produced. This means that the read() member can rely
            /// on being able to read two bytes and reduce the number of pstore accesses
            /// to two for strings < (2^14 - 1) characters (and three for strings longer
            /// than that.
            ///
            /// \param archive  The Archiver to which the value 'str' should be written.
            /// \param str      The string whose content is to be written to the archive.
            /// \returns The value returned by writing the first byte of the string length.
            /// By convention, this is the "address" of the string data (although the precise
            /// meaning is determined by the archive type.

            template <typename Archive>
            static auto write (Archive & archive, value_type const & str) ->
                typename Archive::result_type;

            /// \brief Reads an instance of `sstring_view` from an archiver.
            ///
            /// Reads a string of characters.
            ///
            /// \param archive  The Archiver from which a string will be read.
            /// \param str  A reference to uninitialized memory that is suitable for a new string
            /// instance.
            static void read (::pstore::serialize::archive::database_reader & archive,
                              value_type & str);
        };

        template <typename Archive>
        typename Archive::result_type
        serializer<::pstore::sstring_view>::write (Archive & archive,
                                                   pstore::sstring_view const & str) {
            auto const length = str.length ();

            // Encode the string length as a variable-length integer and emit it.
            std::array<std::uint8_t, varint::max_output_length> encoded_length;
            auto first = std::begin (encoded_length);
            auto last = varint::encode (length, first);
            auto length_bytes = std::distance (first, last);
            assert (length_bytes > 0 &&
                    static_cast<std::size_t> (length_bytes) <= encoded_length.size ());
            if (length_bytes == 1) {
                *(last++) = 0;
            }
            auto const resl =
                serialize::write (archive, ::pstore::gsl::make_span (&(*first), &(*last)));

            // Emit the string body.
            serialize::write (archive, ::pstore::gsl::make_span (str));
            return resl;
        }

        /// \brief A serializer for sstring_view const. It delegates both read and write operations
        ///        to the sstring_view serializer.
        template <>
        struct serializer<::pstore::sstring_view const> {
            using value_type = ::pstore::sstring_view;
            template <typename Archive>
            static auto write (Archive & archive, value_type const & str) ->
                typename Archive::result_type {
                return serializer::write (archive, str);
            }
            template <typename Archive>
            static void read (Archive & archive, value_type & str) {
                serialize::read_uninit (archive, str);
            }
        };

    } // namespace serializer
} // namespace pstore

#endif // PSTORE_SSTRING_VIEW_ARCHIVE_HPP
// eof: include/pstore/sstring_view_archive.hpp
