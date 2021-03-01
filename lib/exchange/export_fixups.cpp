//===- lib/exchange/export_fixups.cpp -------------------------------------===//
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
#include "pstore/exchange/export_fixups.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            gsl::czstring emit_section_name (repo::section_kind const section) noexcept {
                auto const * result = "unknown";
#define X(a)                                                                                       \
    case pstore::repo::section_kind::a: result = #a; break;
                switch (section) {
                    PSTORE_MCREPO_SECTION_KINDS
                case pstore::repo::section_kind::last: break;
                }
#undef X
                return result;
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore
