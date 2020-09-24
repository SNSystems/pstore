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

#include "pstore/exchange/export_fixups.hpp"
#include "pstore/exchange/export_ostream.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/mcrepo/section.hpp"
#include "pstore/support/base64.hpp"

namespace pstore {
    namespace exchange {

        namespace details {

            /// Maps from a stream type to the corresponding OutputIterator that writes successive
            /// objects of type T into stream object for which it was constructed, using operator<<.
            template <typename OStream, typename T>
            struct output_iterator {};
            template <typename T>
            struct output_iterator<std::ostream, T> {
                using type = std::ostream_iterator<T>;
            };
            template <typename T>
            struct output_iterator<std::ostringstream, T> {
                using type = std::ostream_iterator<T>;
            };
            template <typename T>
            struct output_iterator<export_ostream, T> {
                using type = ostream_inserter;
            };

            template <repo::section_kind Kind,
                      typename Content = typename repo::enum_to_section<Kind>::type>
            struct section_exporter {
                static_assert (
                    std::is_same<repo::enum_to_section_t<Kind>, repo::generic_section>::value,
                    "expected enum_to_section_t for Kind to yield generic_section");

                template <typename OStream>
                OStream & operator() (OStream & os, indent const ind, database const & db,
                                      export_name_mapping const & names, Content const & content,
                                      bool comments) {
                    auto const * separator = "";
                    {
                        auto const align = content.align ();
                        if (align != 1U) {
                            os << ind << R"("align":)" << align;
                            separator = ",\n";
                        }
                    }
                    {
                        os << separator << ind << R"("data":")";
                        repo::container<std::uint8_t> const payload = content.payload ();
                        using output_iterator =
                            typename details::output_iterator<OStream, char>::type;
                        to_base64 (std::begin (payload), std::end (payload), output_iterator{os});
                        os << '"';
                    }
                    {
                        repo::container<repo::internal_fixup> const ifx = content.ifixups ();
                        if (!ifx.empty ()) {
                            os << ",\n" << ind << R"("ifixups":)";
                            export_internal_fixups (os, ind, std::begin (ifx), std::end (ifx));
                        }
                    }
                    {
                        repo::container<repo::external_fixup> const xfx = content.xfixups ();
                        if (!xfx.empty ()) {
                            os << ",\n" << ind << R"("xfixups":)";
                            export_external_fixups (os, ind, db, names, std::begin (xfx),
                                                    std::end (xfx), comments);
                        }
                    }
                    os << '\n';
                    return os;
                }
            };

            template <>
            struct section_exporter<repo::section_kind::bss, repo::bss_section> {
                static_assert (std::is_same<repo::enum_to_section_t<repo::section_kind::bss>,
                                            repo::bss_section>::value,
                               "expected enum_to_section_t for bss to yield bss_section");

                template <typename OStream>
                OStream & operator() (OStream & os, indent const ind, database const & /*db*/,
                                      export_name_mapping const & /*names*/,
                                      repo::bss_section const & content, bool const /*comments*/) {
                    auto const * separator = "";
                    {
                        auto const align = content.align ();
                        if (align != 1U) {
                            os << ind << R"("align":)" << align;
                            separator = ",\n";
                        }
                    }
                    os << separator << ind << R"("size":)" << content.size () << '\n';
                    assert (content.ifixups ().empty ());
                    assert (content.xfixups ().empty ());
                    return os;
                }
            };

            template <>
            struct section_exporter<repo::section_kind::debug_line, repo::debug_line_section> {
                static_assert (
                    std::is_same<repo::enum_to_section_t<repo::section_kind::debug_line>,
                                 repo::debug_line_section>::value,
                    "expected enum_to_section_t for debug_line to yield debug_line_section");

                template <typename OStream>
                OStream & operator() (OStream & os, indent const ind, database const & /*db*/,
                                      export_name_mapping const & /*names*/,
                                      repo::debug_line_section const & content,
                                      bool const /*comments*/) {
                    assert (content.align () == 1U);
                    assert (content.xfixups ().size () == 0U);

                    os << ind << R"("header":")" << content.header_digest ().to_hex_string ()
                       << "\",\n";

                    {
                        os << ind << R"("data":")";
                        repo::container<std::uint8_t> const payload = content.payload ();
                        using output_iterator =
                            typename details::output_iterator<OStream, char>::type;
                        to_base64 (std::begin (payload), std::end (payload), output_iterator{os});
                        os << "\",\n";
                    }
                    {
                        repo::container<repo::internal_fixup> const ifixups = content.ifixups ();
                        os << ind << R"("ifixups":)";
                        export_internal_fixups (os, ind, std::begin (ifixups), std::end (ifixups));
                        os << '\n';
                    }
                    return os;
                }
            };

            template <>
            struct section_exporter<repo::section_kind::dependent, repo::dependents> {
                static_assert (std::is_same<repo::enum_to_section_t<repo::section_kind::dependent>,
                                            repo::dependents>::value,
                               "expected enum_to_section_t for dependent to yield dependents");

                template <typename OStream>
                OStream & operator() (OStream & os, indent const ind, database const & db,
                                      export_name_mapping const & names,
                                      repo::dependents const & content, bool const comments) {
                    return emit_array (os, ind, std::begin (content), std::end (content),
                                       [] (OStream & os1, indent const /*ind1*/,
                                           typed_address<repo::compilation_member> const & d) {
                                           // FIXME: represent somehow!
                                       });
                }
            };

        } // end namespace details

        template <repo::section_kind Kind, typename OStream,
                  typename Content = typename repo::enum_to_section<Kind>::type>
        OStream & export_section (OStream & os, indent const ind, database const & db,
                                  export_name_mapping const & names, Content const & content,
                                  bool const comments) {
            os << "{\n";
            details::section_exporter<Kind>{}(os, ind.next (), db, names, content, comments);
            os << ind << '}';
            return os;
        }

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_SECTION_HPP
