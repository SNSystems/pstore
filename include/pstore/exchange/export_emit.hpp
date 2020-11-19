//*                             _                    _ _    *
//*   _____  ___ __   ___  _ __| |_    ___ _ __ ___ (_) |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \ '_ ` _ \| | __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  __/ | | | | | | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___|_| |_| |_|_|\__| *
//*           |_|                                           *
//===- include/pstore/exchange/export_emit.hpp ----------------------------===//
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
/// \file export_emit.hpp
/// \brief Types and functions used in the writing of JSON data such as indents, strings, arrays,
/// and so on.

#ifndef PSTORE_EXCHANGE_EXPORT_EMIT_HPP
#define PSTORE_EXCHANGE_EXPORT_EMIT_HPP

#include "pstore/core/indirect_string.hpp"

namespace pstore {

    class database;

    namespace exchange {
        namespace export_ns {

            class indent {
            public:
                constexpr indent () noexcept = default;
                constexpr indent next () const noexcept { return indent{distance_ + 1U}; }
                constexpr unsigned distance () const noexcept { return distance_; }

            private:
                explicit constexpr indent (unsigned const distance) noexcept
                        : distance_{distance} {}
                unsigned distance_ = 0U;
            };

            template <typename OStream>
            OStream & operator<< (OStream & os, indent const & i) {
                for (unsigned d = i.distance (); d > 0U; --d) {
                    os << "  ";
                }
                return os;
            }

            namespace details {

                template <typename OSStream, typename T>
                void write_span (OSStream & os, gsl::span<T> const & sp) {
                    os.write (sp.data (), sp.length ());
                }

            } // end namespace details

            template <typename OStream, typename Iterator>
            void emit_string (OStream & os, Iterator first, Iterator last) {
                os << '"';
                auto pos = first;
                while ((pos = std::find_if (first, last, [] (char const c) {
                            return c == '"' || c == '\\';
                        })) != last) {
                    details::write_span (os, gsl::make_span (&*first, pos - first));
                    os << '\\' << *pos;
                    first = pos + 1;
                }
                if (first != last) {
                    details::write_span (os, gsl::make_span (&*first, last - first));
                }
                os << '"';
            }

            template <typename OStream>
            void emit_string (OStream & os, raw_sstring_view const & view) {
                emit_string (os, std::begin (view), std::end (view));
            }

            /// Writes an array of values given by the range \p first to \p last to the output
            /// stream \p os. The output follows the JSON syntax of "[ a, b ]" except that each
            /// object
            ///  (a, b) is written on a new line.
            ///
            /// \tparam OStream  An output stream type to which values can be written using the '<<'
            ///     operator.
            /// \tparam InputIt  An input iterator.
            /// \tparam Function  A callable whose signature should be equivalent to:
            ///     void fun(OStream &, indent, std::iterator_traits<InputIt>::value_type const &a);
            /// \param os  An output stream to which values are written using the '<<' operator.
            /// \param ind  The indentation to be applied to each member of the array.
            /// \param first  The start of the range denoting array elements to be emitted.
            /// \param last  The end of the range denoting array elements to be emitted.
            /// \param fn  A function which is called to emit the contents of each object in the
            ///    iterator range denoted by [first, last).
            template <typename OStream, typename InputIt, typename Function>
            OStream & emit_array (OStream & os, indent const ind, InputIt first, InputIt last,
                                  Function fn) {
                auto const * sep = "";
                auto const * tail_sep = "";
                auto tail_sep_indent = indent{};
                os << "[";
                std::for_each (first, last, [&] (auto const & element) {
                    os << sep << '\n';
                    fn (os, ind.next (), element);
                    sep = ",";
                    tail_sep = "\n";
                    tail_sep_indent = ind;
                });
                os << tail_sep << tail_sep_indent << "]";
                return os;
            }

            /// Writes a object to the output stream \p os. The output consists of a pair of braces
            /// with appropriate whitespace. The function \p fn is called to write the properties
            /// and values of the object.
            ///
            /// \tparam OStream  An output stream type to which values can be written using the '<<'
            ///     operator.
            /// \tparam Object  The type of the object to be written.
            /// \tparam Function  A callable whose signature should be equivalent to:
            ///     void fun(OStream &, indent, Object);
            /// \param os  An output stream to which values are written using the '<<' operator.
            /// \param ind  The indentation to be applied to each member of the object.
            /// \param object  The object instance to be written.
            /// \param fn  A function which is called to emit the properties and values of the
            /// object.
            template <typename OStream, typename Object, typename Function>
            OStream & emit_object (OStream & os, indent const ind, Object const & object,
                                   Function fn) {
                os << "{\n";
                fn (os, ind.next (), object);
                os << ind << '}';
                return os;
            }


            namespace details {

                std::string get_string (pstore::database const & db,
                                        pstore::typed_address<pstore::indirect_string> addr);

            } // namespace details

            template <typename OStream>
            OStream & show_string (OStream & os, pstore::database const & db,
                                   pstore::typed_address<pstore::indirect_string> const addr,
                                   bool const comments) {
                if (comments) {
                    auto const str = serialize::read<pstore::indirect_string> (
                        serialize::archive::database_reader{db, addr.to_address ()});
                    os << R"( //")" << str << '"';
                }
                return os;
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_EMIT_HPP
