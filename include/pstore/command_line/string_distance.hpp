//*      _        _                   _ _     _                        *
//*  ___| |_ _ __(_)_ __   __ _    __| (_)___| |_ __ _ _ __   ___ ___  *
//* / __| __| '__| | '_ \ / _` |  / _` | / __| __/ _` | '_ \ / __/ _ \ *
//* \__ \ |_| |  | | | | | (_| | | (_| | \__ \ || (_| | | | | (_|  __/ *
//* |___/\__|_|  |_|_| |_|\__, |  \__,_|_|___/\__\__,_|_| |_|\___\___| *
//*                       |___/                                        *
//===- include/pstore/command_line/string_distance.hpp --------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#ifndef PSTORE_COMMAND_LINE_STRING_DISTANCE_HPP
#define PSTORE_COMMAND_LINE_STRING_DISTANCE_HPP

#include <string>

namespace pstore {
    namespace command_line {

        /// \brief Determine the edit distance between two sequences.
        ///
        /// The algorithm implemented below is the "classic"
        /// dynamic-programming algorithm for computing the Levenshtein distance, which is
        /// described here:
        ///   http://en.wikipedia.org/wiki/Levenshtein_distance
        ///
        /// \param from_array  The first sequence to compare.
        /// \param to_array  The second sequence to compare.
        /// \param max_edit_distance  If non-zero, the maximum edit distance that this
        /// routine is allowed to compute. If the edit distance will exceed that
        /// maximum, returns \c max_edit_distance+1.
        ///
        /// \returns The minimum number of element insertions, removals, or (if
        /// \p allow_replacements is \c true) replacements needed to transform one of
        /// the given sequences into the other. If zero, the sequences are identical.
        std::string::size_type string_distance (std::string const & from_array,
                                                std::string const & to_array,
                                                std::string::size_type max_edit_distance = 0);

    } // namespace command_line
} // namespace pstore

#endif // PSTORE_COMMAND_LINE_STRING_DISTANCE_HPP
