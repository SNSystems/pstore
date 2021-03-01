//===- include/pstore/diff_dump/revision.hpp --------------*- mode: C++ -*-===//
//*                 _     _              *
//*  _ __ _____   _(_)___(_) ___  _ __   *
//* | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* | | |  __/\ V /| \__ \ | (_) | | | | *
//* |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                      *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_DIFF_DUMP_REVISION_HPP
#define PSTORE_DIFF_DUMP_REVISION_HPP

#include <utility>

#include "pstore/core/diff.hpp"
#include "pstore/support/maybe.hpp"

namespace pstore {
    namespace diff_dump {

        using revisions_type = std::pair<revision_number, maybe<revision_number>>;
        revisions_type update_revisions (revisions_type const & revisions,
                                         revision_number const actual_head);

    } // end namespace diff_dump
} // end namespace pstore

#endif // PSTORE_DIFF_DUMP_REVISION_HPP
