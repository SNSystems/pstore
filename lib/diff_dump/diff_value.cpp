//===- lib/diff_dump/diff_value.cpp ---------------------------------------===//
//*      _ _  __  __              _             *
//*   __| (_)/ _|/ _| __   ____ _| |_   _  ___  *
//*  / _` | | |_| |_  \ \ / / _` | | | | |/ _ \ *
//* | (_| | |  _|  _|  \ V / (_| | | |_| |  __/ *
//*  \__,_|_|_| |_|     \_/ \__,_|_|\__,_|\___| *
//*                                             *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/diff_dump/diff_value.hpp"

namespace pstore {
    namespace diff_dump {

        dump::value_ptr make_indices_diff (database & db, revision_number const new_revision,
                                           revision_number const old_revision) {
            PSTORE_ASSERT (new_revision >= old_revision);
            return dump::make_value (
                {make_index_diff<index::name_index> ("names", db, new_revision, old_revision,
                                                     index::get_index<trailer::indices::name>),
                 make_index_diff<index::fragment_index> (
                     "fragments", db, new_revision, old_revision,
                     index::get_index<trailer::indices::fragment>),
                 make_index_diff<index::compilation_index> (
                     "compilations", db, new_revision, old_revision,
                     index::get_index<trailer::indices::compilation>),
                 make_index_diff<index::debug_line_header_index> (
                     "debug_line_headers", db, new_revision, old_revision,
                     index::get_index<trailer::indices::debug_line_header>)});
        }

    } // end namespace diff_dump
} // end namespace pstore
