//*                 _     _                           _    *
//*  _ __ _____   _(_)___(_) ___  _ __     ___  _ __ | |_  *
//* | '__/ _ \ \ / / / __| |/ _ \| '_ \   / _ \| '_ \| __| *
//* | | |  __/\ V /| \__ \ | (_) | | | | | (_) | |_) | |_  *
//* |_|  \___| \_/ |_|___/_|\___/|_| |_|  \___/| .__/ \__| *
//*                                            |_|         *
//===- include/pstore/command_line/revision_opt.hpp -----------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_REVISION_OPT_HPP
#define PSTORE_COMMAND_LINE_REVISION_OPT_HPP

#include "pstore/command_line/option.hpp"
#include "pstore/support/head_revision.hpp"

namespace pstore {
    namespace command_line {

        class revision_opt {
        public:
            revision_opt & operator= (std::string const & val);
            explicit constexpr operator unsigned () const noexcept { return r_; }

        private:
            unsigned r_ = pstore::head_revision;
        };

        template <>
        struct type_description<revision_opt> {
            static gsl::czstring value;
        };

    } // end namespace command_line
} // end namespace pstore

#endif // PSTORE_COMMAND_LINE_REVISION_OPT_HPP
