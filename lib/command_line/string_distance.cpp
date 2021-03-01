//===- lib/command_line/string_distance.cpp -------------------------------===//
//*      _        _                   _ _     _                        *
//*  ___| |_ _ __(_)_ __   __ _    __| (_)___| |_ __ _ _ __   ___ ___  *
//* / __| __| '__| | '_ \ / _` |  / _` | / __| __/ _` | '_ \ / __/ _ \ *
//* \__ \ |_| |  | | | | | (_| | | (_| | \__ \ || (_| | | | | (_|  __/ *
//* |___/\__|_|  |_|_| |_|\__, |  \__,_|_|___/\__\__,_|_| |_|\___\___| *
//*                       |___/                                        *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/string_distance.hpp"

#include <algorithm>
#include <numeric>

#include "pstore/adt/small_vector.hpp"

namespace pstore {
    namespace command_line {

        std::string::size_type string_distance (std::string const & from, std::string const & to,
                                                std::string::size_type const max_edit_distance) {
            using size_type = std::string::size_type;

            size_type const m = from.size ();
            size_type const n = to.size ();

            small_vector<size_type, 64> column (m + 1U);
            std::iota (std::begin (column), std::end (column), size_type{0});
            constexpr auto column_start = size_type{1};

            for (auto x = column_start; x <= n; x++) {
                column[0] = x;
                auto best_this_column = x;
                auto last_diagonal = x - column_start;
                for (auto y = column_start; y <= m; y++) {
                    auto const old_diagonal = column[y];
                    column[y] = std::min ({column[y] + 1U, column[y - 1U] + 1U,
                                           last_diagonal + (from[y - 1U] == to[x - 1U] ? 0U : 1U)});
                    last_diagonal = old_diagonal;
                    best_this_column = std::min (best_this_column, column[y]);
                }
                if (max_edit_distance != 0 && best_this_column > max_edit_distance) {
                    return max_edit_distance + 1U;
                }
            }
            return column[m];
        }

    }     // end namespace command_line
} // end namespace pstore
