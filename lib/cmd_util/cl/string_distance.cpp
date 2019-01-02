//*      _        _                   _ _     _                        *
//*  ___| |_ _ __(_)_ __   __ _    __| (_)___| |_ __ _ _ __   ___ ___  *
//* / __| __| '__| | '_ \ / _` |  / _` | / __| __/ _` | '_ \ / __/ _ \ *
//* \__ \ |_| |  | | | | | (_| | | (_| | \__ \ || (_| | | | | (_|  __/ *
//* |___/\__|_|  |_|_| |_|\__, |  \__,_|_|___/\__\__,_|_| |_|\___\___| *
//*                       |___/                                        *
//===- lib/cmd_util/cl/string_distance.cpp --------------------------------===//
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
#include "pstore/cmd_util/cl/string_distance.hpp"
#include <algorithm>
#include <numeric>
#include "pstore/support/small_vector.hpp"

namespace pstore {
    namespace cmd_util {
        namespace cl {

            std::string::size_type string_distance (std::string const & from,
                                                    std::string const & to,
                                                    std::string::size_type max_edit_distance) {
                using size_type = std::string::size_type;
                size_type const m = from.size ();
                size_type const n = to.size ();

                small_vector<size_type, 64> column (m + 1U);
                std::iota (std::begin (column), std::end (column), size_type{0});
                auto column_start = size_type{1};

                for (auto x = column_start; x <= n; x++) {
                    column[0] = x;
                    auto best_this_column = x;
                    auto last_diagonal = x - column_start;
                    for (auto y = column_start; y <= m; y++) {
                        auto old_diagonal = column[y];
                        column[y] =
                            std::min ({column[y] + 1U, column[y - 1U] + 1U,
                                       last_diagonal + (from[y - 1U] == to[x - 1U] ? 0U : 1U)});
                        last_diagonal = old_diagonal;
                        best_this_column = std::min (best_this_column, column[y]);
                    }
                    if (max_edit_distance && best_this_column > max_edit_distance) {
                        return max_edit_distance + 1U;
                    }
                }
                return column[m];
            }

        } // namespace cl
    }     // namespace cmd_util
} // namespace pstore
