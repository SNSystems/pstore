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
#ifndef PSTORE_EXCHANGE_EXPORT_EMIT_HPP
#define PSTORE_EXCHANGE_EXPORT_EMIT_HPP

#include <algorithm>
#include <string>

#include "pstore/adt/sstring_view.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/core/indirect_string.hpp"

namespace pstore {

    class database;

    namespace exchange {

        class crude_ostream;

#define PSTORE_INDENT "  "
        constexpr auto indent1 = PSTORE_INDENT;
        constexpr auto indent2 = PSTORE_INDENT PSTORE_INDENT;
        constexpr auto indent3 = PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT;
        constexpr auto indent4 = PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT;
        constexpr auto indent5 =
            PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT;
        constexpr auto indent6 =
            PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT;
        constexpr auto indent7 = PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT
            PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT;
        constexpr auto indent8 = PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT
            PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT PSTORE_INDENT;
#undef PSTORE_INDENT

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
            while ((pos = std::find_if (first, last,
                                        [] (char c) { return c == '"' || c == '\\'; })) != last) {
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

        template <typename OStream>
        void emit_string (OStream & os, std::string const & str) {
            emit_string (os, std::begin (str), std::end (str));
        }


        /// Writes an array of values given by the range \p first to \p last to the output
        /// stream \p os. The output follows the JSON syntax of "[ a, b ]" except that each object
        ///  (a, b) is written on a new line.
        ///
        /// \tparam OStream  An output stream type to which values can be written using the '<<'
        ///     operator.
        /// \tparam InputIt  An input iterator.
        /// \tparam Function  A callable whose signature should be equivalent to:
        ///     void fun(OStream &, std::iterator_traits<InputIt>::value_type const &a);
        /// \param os  An output stream to which values are written using the '<<' operator.
        /// \param first  The start of the range denoting array elements to be emitted.
        /// \param last  The end of the range denoting array elements to be emitted.
        /// \param indent  The indentation to be applied to each member of the array.
        /// \param fn  A function which is called to emit the contents of each object in the
        ///    iterator range denoted by [first, last).
        template <typename OStream, typename InputIt, typename Function>
        void emit_array (OStream & os, InputIt first, InputIt last, gsl::czstring indent,
                         Function fn) {
            auto const * sep = "";
            auto const * tail_sep = "";
            auto const * tail_sep_indent = "";
            os << "[";
            std::for_each (first, last, [&] (auto const & element) {
                os << sep << '\n';
                fn (os, element);
                sep = ",";
                tail_sep = "\n";
                tail_sep_indent = indent;
            });
            os << tail_sep << tail_sep_indent << "]";
        }

        void show_string (crude_ostream & os, database const & db,
                          typed_address<indirect_string> addr);

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_EMIT_HPP
