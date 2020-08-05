//*                             _                   _   _              *
//*   _____  ___ __   ___  _ __| |_   ___  ___  ___| |_(_) ___  _ __   *
//*  / _ \ \/ / '_ \ / _ \| '__| __| / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* |  __/>  <| |_) | (_) | |  | |_  \__ \  __/ (__| |_| | (_) | | | | *
//*  \___/_/\_\ .__/ \___/|_|   \__| |___/\___|\___|\__|_|\___/|_| |_| *
//*           |_|                                                      *
//===- include/pstore/exchange/export_section.hpp -------------------------===//
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
#ifndef PSTORE_EXCHANGE_EXPORT_SECTION_HPP
#define PSTORE_EXCHANGE_EXPORT_SECTION_HPP

#include "pstore/mcrepo/fragment.hpp"
#include "pstore/mcrepo/section.hpp"
#include "pstore/support/base64.hpp"
#include "pstore/exchange/export_fixups.hpp"

namespace pstore {
    namespace exchange {

        template <repo::section_kind Kind,
                  typename Content = typename repo::enum_to_section<Kind>::type>
        void emit_section (crude_ostream & os, database const & db,
                           export_name_mapping const & names, Content const & content) {
            {
                os << indent6 << R"("data": ")";
                repo::container<std::uint8_t> const payload = content.payload ();
                to_base64 (std::begin (payload), std::end (payload), ostream_inserter{os});
                os << "\",\n";
            }
            os << indent6 << R"("align": )" << content.align () << ",\n";

            repo::container<repo::internal_fixup> const ifixups = content.ifixups ();
            os << indent6 << R"("ifixups": )";
            export_internal_fixups (os, std::begin (ifixups), std::end (ifixups)) << ",\n";

            repo::container<repo::external_fixup> const xfixups = content.xfixups ();
            os << indent6 << R"("xfixups" :)";
            export_external_fixups (os, db, names, std::begin (xfixups), std::end (xfixups))
                << '\n';
        }

        template <>
        void emit_section<repo::section_kind::bss, repo::bss_section> (
            crude_ostream & os, database const & db, export_name_mapping const & names,
            repo::bss_section const & content) {

            os << indent6 << R"("size": )" << content.size () << ",\n";
            os << indent6 << R"("align": )" << content.align () << ",\n";

            repo::container<repo::internal_fixup> const ifixups = content.ifixups ();
            os << indent6 << R"("ifixups": )";
            export_internal_fixups (os, std::begin (ifixups), std::end (ifixups)) << ",\n";

            repo::container<repo::external_fixup> const xfixups = content.xfixups ();
            os << indent6 << R"("xfixups" :)";
            export_external_fixups (os, db, names, std::begin (xfixups), std::end (xfixups))
                << '\n';
        }

        template <>
        void emit_section<repo::section_kind::debug_line, repo::debug_line_section> (
            crude_ostream & os, database const & /*db*/, export_name_mapping const & /*names*/,
            repo::debug_line_section const & content) {

            assert (content.align () == 1U);
            assert (content.xfixups ().size () == 0U);

            os << indent6 << R"("header": ")" << content.header_digest ().to_hex_string ()
               << "\",\n";

            {
                os << indent6 << R"("data": ")";
                repo::container<std::uint8_t> const payload = content.payload ();
                to_base64 (std::begin (payload), std::end (payload), ostream_inserter{os});
                os << "\",\n";
            }

            repo::container<repo::internal_fixup> const ifixups = content.ifixups ();
            os << indent6 << R"("ifixups": )";
            export_internal_fixups (os, std::begin (ifixups), std::end (ifixups)) << '\n';
        }

        template <>
        void emit_section<repo::section_kind::dependent, repo::dependents> (
            crude_ostream & os, database const & /*db*/, export_name_mapping const & /*names*/,
            repo::dependents const & content) {

            emit_array (
                os, std::begin (content), std::end (content), indent6,
                [] (crude_ostream & os1, typed_address<repo::compilation_member> const & d) {
                    // FIXME: represent as a Relative JSON Pointer
                    // (https://json-schema.org/draft/2019-09/relative-json-pointer.html).
                });
        }


    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_SECTION_HPP
