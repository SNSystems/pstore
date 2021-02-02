//*                             _      __                                      _    *
//*   _____  ___ __   ___  _ __| |_   / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__| |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*           |_|                                   |___/                           *
//===- include/pstore/exchange/export_fragment.hpp ------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_EXPORT_FRAGMENT_HPP
#define PSTORE_EXCHANGE_EXPORT_FRAGMENT_HPP

#include "pstore/exchange/export_section.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            template <typename OStream>
            void emit_fragment (OStream & os, indent const ind, class database const & db,
                                name_mapping const & names,
                                std::shared_ptr<repo::fragment const> const & fragment,
                                bool comments) {
                os << "{\n";
                auto const * section_sep = "";
                for (repo::section_kind const section : *fragment) {
                    auto const object_indent = ind.next ();
                    os << section_sep << object_indent << '"' << emit_section_name (section)
                       << R"(":)";
#define X(a)                                                                                       \
case repo::section_kind::a:                                                                        \
    emit_section<repo::section_kind::a> (                                                          \
        os, object_indent, db, names, fragment->at<pstore::repo::section_kind::a> (), comments);   \
    break;
                    switch (section) {
                        PSTORE_MCREPO_SECTION_KINDS
                    case repo::section_kind::last:
                        // unreachable...
                        PSTORE_ASSERT (false);
                        break;
                    }
#undef X
                    section_sep = ",\n";
                }
                os << '\n' << ind << '}';
            }

            void emit_fragments (ostream & os, indent ind, class database const & db,
                                 unsigned generation, name_mapping const & names, bool comments);

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_FRAGMENT_HPP
