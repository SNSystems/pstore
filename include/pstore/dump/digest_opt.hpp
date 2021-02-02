//*      _ _                 _                 _    *
//*   __| (_) __ _  ___  ___| |_    ___  _ __ | |_  *
//*  / _` | |/ _` |/ _ \/ __| __|  / _ \| '_ \| __| *
//* | (_| | | (_| |  __/\__ \ |_  | (_) | |_) | |_  *
//*  \__,_|_|\__, |\___||___/\__|  \___/| .__/ \__| *
//*          |___/                      |_|         *
//===- include/pstore/dump/digest_opt.hpp ---------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_DUMP_DIGEST_OPT_HPP
#define PSTORE_DUMP_DIGEST_OPT_HPP

#include "pstore/core/index_types.hpp"
#include "pstore/command_line/command_line.hpp"
#include "pstore/support/maybe.hpp"

namespace pstore {
    namespace dump {

        class digest_opt {
        public:
            constexpr digest_opt () noexcept = default;
            explicit constexpr digest_opt (index::digest const & d) noexcept
                    : d_{d} {}
            explicit constexpr operator index::digest () const noexcept { return d_; }

        private:
            index::digest d_;
        };

    } // end namespace dump
} // end namespace pstore

#endif // PSTORE_DUMP_DIGEST_OPT_HPP
