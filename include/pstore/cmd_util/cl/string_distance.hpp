//*      _        _                   _ _     _                        *
//*  ___| |_ _ __(_)_ __   __ _    __| (_)___| |_ __ _ _ __   ___ ___  *
//* / __| __| '__| | '_ \ / _` |  / _` | / __| __/ _` | '_ \ / __/ _ \ *
//* \__ \ |_| |  | | | | | (_| | | (_| | \__ \ || (_| | | | | (_|  __/ *
//* |___/\__|_|  |_|_| |_|\__, |  \__,_|_|___/\__\__,_|_| |_|\___\___| *
//*                       |___/                                        *
//===- include/pstore/cmd_util/cl/string_distance.hpp ---------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

#ifndef PSTORE_CMD_UTIL_CL_STRING_DISTANCE_HPP
#define PSTORE_CMD_UTIL_CL_STRING_DISTANCE_HPP

#include <string>

namespace pstore {
    namespace cmd_util {
        namespace cl {

            /// \brief Determine the edit distance between two sequences.
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

        } // namespace cl
    }     // namespace cmd_util
} // namespace pstore

#endif // PSTORE_CMD_UTIL_CL_STRING_DISTANCE_HPP
// eof: include/pstore/cmd_util/cl/string_distance.hpp
