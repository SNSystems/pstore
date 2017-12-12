//*      _                  _               _   _                          *
//*  ___| |_ __ _ _ __   __| | __ _ _ __ __| | | |_ _   _ _ __   ___  ___  *
//* / __| __/ _` | '_ \ / _` |/ _` | '__/ _` | | __| | | | '_ \ / _ \/ __| *
//* \__ \ || (_| | | | | (_| | (_| | | | (_| | | |_| |_| | |_) |  __/\__ \ *
//* |___/\__\__,_|_| |_|\__,_|\__,_|_|  \__,_|  \__|\__, | .__/ \___||___/ *
//*                                                 |___/|_|               *
//===- include/pstore/serialize/standard_types.hpp ------------------------===//
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
/// \file standard_types.hpp
/// \brief Provides serialization capabilities for common types.

#ifndef PSTORE_SERIALIZE_STANDARD_TYPES_HPP
#define PSTORE_SERIALIZE_STANDARD_TYPES_HPP

#include <array>
#include <atomic>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <type_traits>

#include "pstore/serialize/common.hpp"
#include "pstore/serialize/types.hpp"
#include "pstore/varint.hpp"

///@{
/// \name Reading and writing standard types
/// A collection of convenience methods which each know how to serialize the types defined by the
/// standard library (string, vector, set, etc.)
namespace pstore {
    namespace serialize {

        struct string_helper {
            template <typename Archive, typename StringType>
            static auto write (Archive & archive, StringType const & str) ->
                typename Archive::result_type {
                auto const length = str.length ();

                // Encode the string length as a variable-length integer.
                std::array<std::uint8_t, varint::max_output_length> encoded_length;
                auto first = std::begin (encoded_length);
                auto last = varint::encode (length, first);
                auto length_bytes = std::distance (first, last);
                assert (length_bytes > 0 &&
                        static_cast<std::size_t> (length_bytes) <= encoded_length.size ());
                if (length_bytes == 1) {
                    *(last++) = 0;
                }
                // Emit the string length.
                auto const resl =
                    serialize::write (archive, ::pstore::gsl::make_span (&(*first), &(*last)));

                // Emit the string body.
                serialize::write (archive, ::pstore::gsl::make_span (str));
                return resl;
            }

            template <typename Archive>
            static std::size_t read_length (Archive & archive) {
                std::array<std::uint8_t, varint::max_output_length> encoded_length{{0}};
                // First read the two initial bytes. These contain the variable length value
                // but might not be enough for the entire value.
                static_assert (varint::max_output_length >= 2,
                               "maximum encoded varint length must be >= 2");
                serialize::read_uninit (archive,
                                        ::pstore::gsl::make_span (encoded_length.data (), 2));

                auto const varint_length = varint::decode_size (std::begin (encoded_length));
                assert (varint_length > 0);
                // Was that initial read of 2 bytes enough? If not get the rest of the
                // length value.
                if (varint_length > 2) {
                    assert (varint_length <= encoded_length.size ());
                    serialize::read_uninit (
                        archive,
                        ::pstore::gsl::make_span (encoded_length.data () + 2, varint_length - 2));
                }

                return varint::decode (encoded_length.data (), varint_length);
            }
        };

        /// \brief A serializer for std::string.
        template <>
        struct serializer<std::string> {
            using value_type = std::string;

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
                typename Archive::result_type {
                return string_helper::write (archive, str);
            }

            /// \brief Reads an instance of `std::string` from an archiver.
            ///
            /// Reads an string of characters.
            ///
            /// \param archive  The Archiver from which a string will be read.
            /// \param str      A reference to uninitialized memory that is suitable for a new
            /// string instance.

            template <typename Archive>
            static void read (Archive & archive, value_type & str) {
                // Read the body of the string.
                new (&str) value_type;

                // Deleter will ensure that the string is destroyed on exit if an exception is
                // raised here.
                auto dtor = [](value_type * p) {
                    using namespace std;
                    p->~string ();
                };
                std::unique_ptr<value_type, decltype (dtor)> deleter (&str, dtor);

                auto const length = string_helper::read_length (archive);
                str.resize (length);

                // Now read the body of the string.
                // FIXME: const_cast will not be necessary with the C++17 standard library.
                serialize::read_uninit (
                    archive, ::pstore::gsl::make_span (const_cast<char *> (str.data ()),
                                                       static_cast<std::ptrdiff_t> (length)));

                // Release ownership from the deleter so that the initialized object is returned to
                // the caller.
                deleter.release ();
            }
        };

        /// \brief A serializer for std::string const. It delegates both read and write operations
        ///        to the std::string serializer.
        template <>
        struct serializer<std::string const> {
            using value_type = std::string;
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

        /// \brief A helper class which can be used to emit containers which have a size() method
        /// and
        ///        which support the requirements for range-based 'for'.
        template <typename Container>
        struct container_archive_helper {

            /// \brief Writes the contents of a container to an archive.
            ///
            /// Writes an initial std::size_t value with the number of elements in the container
            /// followed by an array of of those elements, in the order returned by iteration.
            ///
            /// \param archive The archive to which the container is to be serialized.
            /// \param ty The container whose contents are to be written.

            template <typename Archive>
            static auto write (Archive & archive, Container const & ty) ->
                typename Archive::result_type {
                auto result = serialize::write (archive, std::size_t{ty.size ()});
                for (typename Container::value_type const & m : ty) {
                    serialize::write (archive, m);
                }
                return result;
            }

            using insert_callback = std::function<void(typename Container::value_type const &)>;

            /// \brief Reads the contents of a container from an archive.
            ///
            /// Reads a std::size_t value -- the number of following elements -- and an array of
            /// Container::vaue_type elements. For each element, the inserter function is invoked:
            /// it is passed the container and value to be inserted. Its job is simply to insert
            /// the value into the given container.
            ///
            /// \param archive The archive from which the container will be read.
            /// \param inserter A function which is responsible for inserting each of the
            ///                 Container::value_type elements from the archive into the container.

            template <typename Archive>
            static void read (Archive & archive, insert_callback inserter) {
                Container result;
                auto num_members = serialize::read<std::size_t> (archive);
                while (num_members--) {
                    inserter (serialize::read<typename Container::value_type> (archive));
                }
            }
        };


        /// \brief A serializer for std::atomic<T>
        template <typename T>
        struct serializer<std::atomic<T>> {
            using value_type = std::atomic<T>;

            /// \brief Writes an instance of `std::atomic<>` to an archive.
            ///
            /// The data stream format follows that of the underlying type.
            ///
            /// \param archive  The archive to which the atomic will be written.
            /// \param value    The `std::atomic<>` instance that is to be serialized.
            template <typename Archive>
            static auto write (Archive & archive, value_type const & value) ->
                typename Archive::result_type {
                return serialize::write (archive, value.load ());
            }

            /// \brief Reads an instance of `std::atomic<>` from an archive.
            ///
            /// \param archive  The archiver from which the value will be read.
            /// \param value  The de-serialized std::atomic value.
            template <typename Archive>
            static void read (Archive & archive, value_type & value) {
                serialize::read_uninit<T> (archive, value);
            }
        };


        /// \brief A serializer for std::pair<T,U>
        template <typename T, typename U>
        struct serializer<std::pair<T, U>> {
            using value_type = std::pair<T, U>;

            /// \brief Writes an instance of `std::pair<>` to an archive.
            ///
            /// The data stream format consists of the two pair elements, first
            /// and second, read and written in that order.
            ///
            /// \param archive  The archive to which the pair will be written.
            /// \param value    The `std::pair<>` instance that is to be serialized.
            template <typename Archive>
            static auto write (Archive & archive, value_type const & value) ->
                typename Archive::result_type {
                auto result = serialize::write (archive, value.first);
                serialize::write (archive, value.second);
                return result;
            }

            /// \brief Reads an instance of `std::pair<>` from an archive.
            ///
            /// \param archive  The archiver from which the value will be read.
            /// \param value  A reference to uninitialized memory into which the de-serialized pair
            /// will be read.
            template <typename Archive>
            static void read (Archive & archive, value_type & value) {
                serialize::read_uninit (archive, value.first);
                serialize::read_uninit (archive, value.second);
            }
        };
    } // namespace serialize
} // namespace pstore
///@}
#endif // PSTORE_SERIALIZE_STANDARD_TYPES_HPP
// eof: include/pstore/serialize/standard_types.hpp
