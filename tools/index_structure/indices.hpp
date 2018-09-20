//*  _           _ _                *
//* (_)_ __   __| (_) ___ ___  ___  *
//* | | '_ \ / _` | |/ __/ _ \/ __| *
//* | | | | | (_| | | (_|  __/\__ \ *
//* |_|_| |_|\__,_|_|\___\___||___/ *
//*                                 *
//===- tools/index_structure/indices.hpp ----------------------------------===//
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
#ifndef PSTORE_INDEX_STRUCTURE_INDICES_HPP
#define PSTORE_INDEX_STRUCTURE_INDICES_HPP 1

#include <bitset>
#include <type_traits>
#include <vector>
#include "pstore/core/database.hpp"
#include "pstore/core/index_types.hpp"

#define INDICES                                                                                    \
    X (digest)                                                                                     \
    X (ticket)                                                                                     \
    X (name)                                                                                       \
    X (write)                                                                                      \
    X (debug_line_header)

#define X(a) a,
enum class indices : unsigned { INDICES last };
#undef X

template <indices IndexName>
struct index_accessor {};

template <>
struct index_accessor<indices::digest> {
    static decltype (&pstore::index::get_digest_index) get () {
        return &pstore::index::get_digest_index;
    }
};

template <>
struct index_accessor<indices::ticket> {
    static decltype (&pstore::index::get_ticket_index) get () {
        return &pstore::index::get_ticket_index;
    }
};

template <>
struct index_accessor<indices::name> {
    static decltype (&pstore::index::get_name_index) get () {
        return &pstore::index::get_name_index;
    }
};

template <>
struct index_accessor<indices::write> {
    static decltype (&pstore::index::get_write_index) get () {
        return &pstore::index::get_write_index;
    }
};

template <>
struct index_accessor<indices::debug_line_header> {
    static decltype (&pstore::index::get_debug_line_header_index) get () {
        return &pstore::index::get_debug_line_header_index;
    }
};

using indices_bitset =
    std::bitset<static_cast<std::underlying_type<indices>::type> (indices::last)>;
extern std::vector<char const *> const index_names;

bool set_from_name (indices_bitset * const bs, std::string const & name);

#endif // PSTORE_INDEX_STRUCTURE_INDICES_HPP
// eof:tools/index_structure/indices.hpp

