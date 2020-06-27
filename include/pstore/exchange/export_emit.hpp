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


namespace pstore {
    namespace exchange {

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

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_EMIT_HPP
