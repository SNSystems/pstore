//*                 _     _              *
//*  _ __ _____   _(_)___(_) ___  _ __   *
//* | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* | | |  __/\ V /| \__ \ | (_) | | | | *
//* |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                      *
//===- lib/diff_dump/revision.cpp -----------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/diff_dump/revision.hpp"

#include "pstore/core/diff.hpp"
#include "pstore/support/head_revision.hpp"

namespace pstore {
    namespace diff_dump {

        revisions_type update_revisions (revisions_type const & revisions,
                                         revision_number const actual_head) {
            revision_number r1 = revisions.first;
            maybe<revision_number> r2 = revisions.second;

            if (r1 == pstore::head_revision) {
                r1 = actual_head;
            }
            if (r2) {
                if (*r2 == pstore::head_revision) {
                    r2 = actual_head;
                }
            } else {
                r2 = r1 > 0 ? r1 - 1 : 0;
            }
            if (r1 < *r2) {
                std::swap (r1, *r2);
            }
            return std::make_pair (r1, r2);
        }

    } // end namespace diff_dump
} // end namespace pstore
