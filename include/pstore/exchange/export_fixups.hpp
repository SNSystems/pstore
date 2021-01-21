//*                             _      __ _                       *
//*   _____  ___ __   ___  _ __| |_   / _(_)_  ___   _ _ __  ___  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | |_| \ \/ / | | | '_ \/ __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  _| |>  <| |_| | |_) \__ \ *
//*  \___/_/\_\ .__/ \___/|_|   \__| |_| |_/_/\_\\__,_| .__/|___/ *
//*           |_|                                     |_|         *
//===- include/pstore/exchange/export_fixups.hpp --------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_EXPORT_FIXUPS_HPP
#define PSTORE_EXCHANGE_EXPORT_FIXUPS_HPP

#include "pstore/exchange/export_names.hpp"
#include "pstore/mcrepo/generic_section.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            gsl::czstring emit_section_name (repo::section_kind section) noexcept;

            template <typename OStream, typename IFixupIterator>
            OStream & emit_internal_fixups (OStream & os, indent const ind, IFixupIterator first,
                                            IFixupIterator last) {
                emit_array (
                    os, ind, first, last,
                    [] (OStream & os1, indent const ind1, repo::internal_fixup const & ifx) {
                        os1 << ind1 << '{' << R"("section":")" << emit_section_name (ifx.section)
                            << R"(","type":)" << static_cast<unsigned> (ifx.type) << ','
                            << R"("offset":)" << ifx.offset << ',' << R"("addend":)" << ifx.addend
                            << '}';
                    });
                return os;
            }

            template <typename OStream, typename XFixupIterator>
            OStream & emit_external_fixups (OStream & os, indent const ind, database const & db,
                                            name_mapping const & names, XFixupIterator first,
                                            XFixupIterator last, bool comments) {
                return emit_array_with_name (os, ind, db, first, last, comments,
                                             [&] (OStream & os1, repo::external_fixup const & xfx) {
                                                 os1 << R"({"name":)" << names.index (xfx.name)
                                                     << R"(,"type":)"
                                                     << static_cast<unsigned> (xfx.type)
                                                     << R"(,"offset":)" << xfx.offset
                                                     << R"(,"addend":)" << xfx.addend << '}';
                                                 return xfx.name;
                                             });
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_FIXUPS_HPP
