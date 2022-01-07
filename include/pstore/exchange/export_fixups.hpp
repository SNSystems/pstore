//===- include/pstore/exchange/export_fixups.hpp ----------*- mode: C++ -*-===//
//*                             _      __ _                       *
//*   _____  ___ __   ___  _ __| |_   / _(_)_  ___   _ _ __  ___  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | |_| \ \/ / | | | '_ \/ __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  _| |>  <| |_| | |_) \__ \ *
//*  \___/_/\_\ .__/ \___/|_|   \__| |_| |_/_/\_\\__,_| .__/|___/ *
//*           |_|                                     |_|         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file export_fixups.hpp
/// \brief Code to emit internal- and external-fixups for export.

#ifndef PSTORE_EXCHANGE_EXPORT_FIXUPS_HPP
#define PSTORE_EXCHANGE_EXPORT_FIXUPS_HPP

#include "pstore/exchange/export_strings.hpp"
#include "pstore/mcrepo/generic_section.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            gsl::czstring emit_section_name (repo::section_kind section) noexcept;

            template <typename IFixupIterator>
            ostream_base & emit_internal_fixups (ostream_base & os, indent const ind,
                                                 IFixupIterator first, IFixupIterator last) {
                emit_array (
                    os, ind, first, last,
                    [] (ostream_base & os1, indent const ind1, repo::internal_fixup const & ifx) {
                        os1 << ind1 << '{' << R"("section":")" << emit_section_name (ifx.section)
                            << R"(","type":)" << static_cast<unsigned> (ifx.type) << ','
                            << R"("offset":)" << ifx.offset << ',' << R"("addend":)" << ifx.addend
                            << '}';
                    });
                return os;
            }

            template <typename XFixupIterator>
            ostream_base &
            emit_external_fixups (ostream_base & os, indent const ind, database const & db,
                                  string_mapping const & strings, XFixupIterator first,
                                  XFixupIterator last, bool comments) {
                return emit_array_with_name (os, ind, db, first, last, comments,
                                             [&] (ostream_base & os1,
                                                  repo::external_fixup const & xfx) {
                                                 os1 << R"({"name":)" << strings.index (xfx.name)
                                                     << R"(,"type":)"
                                                     << static_cast<unsigned> (xfx.type);
                                                 if (xfx.is_weak) {
                                                     os1 << R"(,"is_weak":)" << xfx.is_weak;
                                                 }
                                                 os1 << R"(,"offset":)" << xfx.offset;
                                                 if (xfx.addend != 0) {
                                                     os1 << R"(,"addend":)" << xfx.addend;
                                                 }
                                                 os1 << '}';
                                                 return xfx.name;
                                             });
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_FIXUPS_HPP
