//*                _   _              *
//*  ___  ___  ___| |_(_) ___  _ __   *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* \__ \  __/ (__| |_| | (_) | | | | *
//* |___/\___|\___|\__|_|\___/|_| |_| *
//*                                   *
//===- lib/mcrepo/section.cpp ---------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/mcrepo/section.hpp"

#include <ostream>

namespace pstore {
    namespace repo {

        std::ostream & operator<< (std::ostream & os, section_kind const kind) {
            auto str = "unknown section_kind";
#define X(a)                                                                                       \
    case section_kind::a: str = #a; break;
            switch (kind) {
                PSTORE_MCREPO_SECTION_KINDS
            case section_kind::last: break;
            }
            return os << str;
#undef X
        }

        section_creation_dispatcher::~section_creation_dispatcher () noexcept = default;
        dispatcher::~dispatcher () noexcept = default;

    } // end namespace repo
} // end namespace pstore
