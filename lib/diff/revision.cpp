//*                 _     _              *
//*  _ __ _____   _(_)___(_) ___  _ __   *
//* | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* | | |  __/\ V /| \__ \ | (_) | | | | *
//* |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                      *
//===- lib/diff/revision.cpp ----------------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
//===----------------------------------------------------------------------===//

#include "pstore/diff/revision.hpp"
#include "pstore/support/head_revision.hpp"

namespace pstore {
    namespace diff {

        revisions_type update_revisions (revisions_type const revisions,
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

    } // namespace diff
} // namespace pstore
