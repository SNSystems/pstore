//*                             _      __ _                       *
//*   _____  ___ __   ___  _ __| |_   / _(_)_  ___   _ _ __  ___  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | |_| \ \/ / | | | '_ \/ __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  _| |>  <| |_| | |_) \__ \ *
//*  \___/_/\_\ .__/ \___/|_|   \__| |_| |_/_/\_\\__,_| .__/|___/ *
//*           |_|                                     |_|         *
//===- lib/exchange/export_fixups.hpp -------------------------------------===//
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
#ifndef PSTORE_EXCHANGE_EXPORT_FIXUPS_HPP
#define PSTORE_EXCHANGE_EXPORT_FIXUPS_HPP

#include "pstore/exchange/export_emit.hpp"
#include "pstore/exchange/export_names.hpp"
#include "pstore/mcrepo/generic_section.hpp"

namespace pstore {
    namespace exchange {

        gsl::czstring section_name (repo::section_kind section) noexcept;

        template <typename OStream, typename IFixupIterator>
        OStream & emit_section_ifixups (OStream & os, IFixupIterator first, IFixupIterator last) {
            emit_array (
                os, first, last, indent6, [] (OStream & os1, repo::internal_fixup const & ifx) {
                    os1 << indent7 << "{\n";
                    os1 << indent8 << R"("section": ")" << section_name (ifx.section) << "\",\n";
                    os1 << indent8 << R"("type": )" << static_cast<unsigned> (ifx.type) << ",\n";
                    os1 << indent8 << R"("offset": )" << ifx.offset << ",\n";
                    os1 << indent8 << R"("addend": )" << ifx.addend << '\n';
                    os1 << indent7 << '}';
                });
            return os;
        }

        template <typename OStream, typename XFixupIterator>
        OStream & emit_section_xfixups (OStream & os, database const & db,
                                        name_mapping const & names, XFixupIterator first,
                                        XFixupIterator last) {
            emit_array (
                os, first, last, indent6, [&] (OStream & os1, repo::external_fixup const & xfx) {
                    os1 << indent7 << "{\n";
                    os1 << indent8 << R"("name": ")" << names.index (xfx.name) << ',';
                    show_string (os1, db, xfx.name);
                    os1 << '\n';
                    os1 << indent8 << R"("type": )" << static_cast<unsigned> (xfx.type) << ",\n";
                    os1 << indent8 << R"("offset": )" << xfx.offset << ",\n";
                    os1 << indent8 << R"("addend": )" << xfx.addend << '\n';
                    os1 << indent7 << '}';
                });
            return os;
        }

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_FIXUPS_HPP
