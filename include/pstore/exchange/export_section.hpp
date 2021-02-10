//*                             _                   _   _              *
//*   _____  ___ __   ___  _ __| |_   ___  ___  ___| |_(_) ___  _ __   *
//*  / _ \ \/ / '_ \ / _ \| '__| __| / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* |  __/>  <| |_) | (_) | |  | |_  \__ \  __/ (__| |_| | (_) | | | | *
//*  \___/_/\_\ .__/ \___/|_|   \__| |___/\___|\___|\__|_|\___/|_| |_| *
//*           |_|                                                      *
//===- include/pstore/exchange/export_section.hpp -------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_EXPORT_SECTION_HPP
#define PSTORE_EXCHANGE_EXPORT_SECTION_HPP

#include "pstore/exchange/export_fixups.hpp"
#include "pstore/exchange/export_ostream.hpp"
#include "pstore/mcrepo/fragment.hpp"
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
                struct output_iterator<ostream_base, T> {
                    using type = ostream_inserter;
                };
                template <typename T>
                struct output_iterator<ostream, T> {
                    using type = ostream_inserter;
                };
                template <typename T>
                struct output_iterator<ostringstream, T> {
                    using type = ostream_inserter;
                };

                template <typename Content>
                struct section_content_exporter;
                template <>
                struct section_content_exporter<repo::generic_section> {
                    template <typename OStream>
                    OStream & operator() (OStream & os, indent const ind, class database const & db,
                                          name_mapping const & names,
                                          repo::generic_section const & content,
                                          bool const comments) {
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
                                PSTORE_ASSERT (content1.ifixups ().empty ());
                                PSTORE_ASSERT (content1.xfixups ().empty ());
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
                                PSTORE_ASSERT (content1.align () == 1U);
                                PSTORE_ASSERT (content1.xfixups ().size () == 0U);
                                os1 << ind1 << R"("header":)";
                                emit_digest (os1, content1.header_digest ());
                                os1 << ",\n";
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
                                               os1 << ind1 << '{' << R"("compilation":)";
                                               emit_digest (os1, d.compilation);
                                               os1 << R"(,"index":)" << d.index << '}';
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
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_SECTION_HPP
