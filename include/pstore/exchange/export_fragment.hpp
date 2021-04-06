//===- include/pstore/exchange/export_fragment.hpp --------*- mode: C++ -*-===//
//*                             _      __                                      _    *
//*   _____  ___ __   ___  _ __| |_   / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__| |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*           |_|                                   |___/                           *
//===----------------------------------------------------------------------===//
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

            void emit_fragment (ostream_base & os, indent const ind, class database const & db,
                                string_mapping const & strings,
                                std::shared_ptr<repo::fragment const> const & fragment,
                                bool comments);

            void emit_fragments (ostream & os, indent ind, class database const & db,
                                 unsigned generation, string_mapping const & strings,
                                 bool comments);

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_FRAGMENT_HPP
