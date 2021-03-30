//===- lib/exchange/export_paths.cpp --------------------------------------===//
//*                             _                 _   _          *
//*   _____  ___ __   ___  _ __| |_   _ __   __ _| |_| |__  ___  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | '_ \ / _` | __| '_ \/ __| *
//* |  __/>  <| |_) | (_) | |  | |_  | |_) | (_| | |_| | | \__ \ *
//*  \___/_/\_\ .__/ \___/|_|   \__| | .__/ \__,_|\__|_| |_|___/ *
//*           |_|                    |_|                         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file export_paths.cpp
/// \brief  Exporting the members of the paths index.

#include "pstore/exchange/export_paths.hpp"

#include <cassert>
#include <limits>

#include "pstore/core/diff.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/exchange/export_ostream.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            void emit_paths (ostream_base & os, indent const ind, database const & db,
                             unsigned const generation,
                             gsl::not_null<name_mapping *> const string_table) {
                auto const paths_index = index::get_index<trailer::indices::path> (db);
                PSTORE_ASSERT (generation > 0);

                auto const * separator = "";
                auto const * tail_separator = "";
                auto close_bracket_indent = indent{};

                auto const member_indent = ind.next ();
                auto const out_fn = [&] (pstore::address const addr) {
                    os << separator << '\n'
                       << member_indent
                       << string_table->index (typed_address<indirect_string>::make (addr));

                    separator = ",";
                    tail_separator = "\n";
                    close_bracket_indent = ind;
                };
                os << '[';
                diff (db, *paths_index, generation - 1U, make_diff_out (&out_fn));
                os << tail_separator << close_bracket_indent << ']';
            }

        } // end namespace export_ns
    }     // end namespace exchange
} // end namespace pstore
