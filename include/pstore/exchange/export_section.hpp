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
        namespace export_ns {

            namespace details {

                /// Maps from a stream type to the corresponding OutputIterator that writes
                /// successive objects of type T into stream object for which it was constructed,
                /// using operator<<.
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
                struct output_iterator<ostream, T> {
                    using type = ostream_inserter;
                };

                template <typename Content>
                struct section_content_exporter;
                template <>
                struct section_content_exporter<repo::generic_section> {
                    template <typename OStream>
                    OStream & operator() (OStream & os, indent const ind, class database const & db,
                                          name_mapping const & names,
                                          repo::generic_section const & content, bool comments) {
                        return emit_object (
                            os, ind, content,
                            [&db, &names, comments] (OStream & os1, indent const ind1,
                                                     repo::generic_section const & content1) {
                                auto const * separator = "";
                                {
                                    auto const align = content1.align ();
                                    if (align != 1U) {
                                        os1 << ind1 << R"("align":)" << align;
                                        separator = ",\n";
                                    }
                                }
                                {
                                    os1 << separator << ind1 << R"("data":")";
                                    repo::container<std::uint8_t> const payload =
                                        content1.payload ();
                                    using output_iterator =
                                        typename details::output_iterator<OStream, char>::type;
                                    to_base64 (std::begin (payload), std::end (payload),
                                               output_iterator{os1});
                                    os1 << '"';
                                }
                                {
                                    repo::container<repo::internal_fixup> const ifx =
                                        content1.ifixups ();
                                    if (!ifx.empty ()) {
                                        os1 << ",\n" << ind1 << R"("ifixups":)";
                                        emit_internal_fixups (os1, ind1, std::begin (ifx),
                                                              std::end (ifx));
                                    }
                                }
                                {
                                    repo::container<repo::external_fixup> const xfx =
                                        content1.xfixups ();
                                    if (!xfx.empty ()) {
                                        os1 << ",\n" << ind1 << R"("xfixups":)";
                                        emit_external_fixups (os1, ind1, db, names,
                                                              std::begin (xfx), std::end (xfx),
                                                              comments);
                                    }
                                }
                                os1 << '\n';
                            });
                    }
                };

                template <>
                struct section_content_exporter<repo::bss_section> {
                    using bsss = repo::bss_section;

                    template <typename OStream>
                    OStream & operator() (OStream & os, indent const ind,
                                          class database const & /*db*/,
                                          name_mapping const & /*names*/, bsss const & content,
                                          bool const /*comments*/) {
                        return emit_object (
                            os, ind, content,
                            [] (OStream & os1, indent const ind1, bsss const & content1) {
                                auto const * separator = "";
                                {
                                    auto const align = content1.align ();
                                    if (align != 1U) {
                                        os1 << ind1 << R"("align":)" << align;
                                        separator = ",\n";
                                    }
                                }
                                os1 << separator << ind1 << R"("size":)" << content1.size ()
                                    << '\n';
                                assert (content1.ifixups ().empty ());
                                assert (content1.xfixups ().empty ());
                            });
                    }
                };

                template <>
                struct section_content_exporter<repo::debug_line_section> {
                    using dls = repo::debug_line_section;

                    template <typename OStream>
                    OStream & operator() (OStream & os, indent const ind,
                                          class database const & /*db*/,
                                          name_mapping const & /*names*/, dls const & content,
                                          bool const /*comments*/) {
                        return emit_object (
                            os, ind, content,
                            [] (OStream & os1, indent const ind1, dls const & content1) {
                                assert (content1.align () == 1U);
                                assert (content1.xfixups ().size () == 0U);
                                os1 << ind1 << R"("header":")"
                                    << content1.header_digest ().to_hex_string () << "\",\n";
                                {
                                    os1 << ind1 << R"("data":")";
                                    repo::container<std::uint8_t> const payload =
                                        content1.payload ();
                                    using output_iterator =
                                        typename details::output_iterator<OStream, char>::type;
                                    to_base64 (std::begin (payload), std::end (payload),
                                               output_iterator{os1});
                                    os1 << "\",\n";
                                }
                                {
                                    repo::container<repo::internal_fixup> const ifixups =
                                        content1.ifixups ();
                                    os1 << ind1 << R"("ifixups":)";
                                    emit_internal_fixups (os1, ind1, std::begin (ifixups),
                                                          std::end (ifixups));
                                    os1 << '\n';
                                }
                            });
                    }
                };

                template <>
                struct section_content_exporter<repo::linked_definitions> {
                    template <typename OStream>
                    OStream &
                    operator() (OStream & os, indent const ind, class database const & /*db*/,
                                name_mapping const & /*names*/,
                                repo::linked_definitions const & content, bool const /*comments*/) {
                        return emit_array (os, ind, std::begin (content), std::end (content),
                                           [] (OStream & os1, indent const ind1,
                                               repo::linked_definitions::value_type const & d) {
                                               os1 << ind1 << '{' << R"("compilation":")"
                                                   << d.compilation.to_hex_string ()
                                                   << R"(","index":)" << d.index << '}';
                                           });
                    }
                };

                template <repo::section_kind Kind,
                          typename Content = typename repo::enum_to_section<Kind>::type>
                struct section_exporter {
                    template <typename OStream>
                    OStream & operator() (OStream & os, indent const ind, class database const & db,
                                          name_mapping const & names, Content const & content,
                                          bool comments) {
                        return section_content_exporter<Content>{}(os, ind, db, names, content,
                                                                   comments);
                    }
                };

            } // end namespace details

            template <repo::section_kind Kind, typename OStream,
                      typename Content = typename repo::enum_to_section<Kind>::type>
            OStream & emit_section (OStream & os, indent const ind, class database const & db,
                                    name_mapping const & names, Content const & content,
                                    bool const comments) {
                return details::section_exporter<Kind>{}(os, ind, db, names, content, comments);
            }

        } // end namespace export_ns
    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_SECTION_HPP
