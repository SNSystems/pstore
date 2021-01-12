//*            _     _                              _    *
//*   __ _  __| | __| |   _____  ___ __   ___  _ __| |_  *
//*  / _` |/ _` |/ _` |  / _ \ \/ / '_ \ / _ \| '__| __| *
//* | (_| | (_| | (_| | |  __/>  <| |_) | (_) | |  | |_  *
//*  \__,_|\__,_|\__,_|  \___/_/\_\ .__/ \___/|_|   \__| *
//*                               |_|                    *
//*      _        _                  *
//*  ___| |_ _ __(_)_ __   __ _ ___  *
//* / __| __| '__| | '_ \ / _` / __| *
//* \__ \ |_| |  | | | | | (_| \__ \ *
//* |___/\__|_|  |_|_| |_|\__, |___/ *
//*                       |___/      *
//===- unittests/exchange/add_export_strings.hpp --------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_UNITTESTS_EXCHANGE_ADD_EXPORT_STRINGS_HPP
#define PSTORE_UNITTESTS_EXCHANGE_ADD_EXPORT_STRINGS_HPP

#include <algorithm>
#include <vector>

#include "pstore/core/hamt_set.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/indirect_string.hpp"
#include "pstore/core/transaction.hpp"

#include "empty_store.hpp"

/// Add a collection of czstrings to the database and yields a collection which
/// holds the mapping for each string to its indirect-address in the database. These strings
/// are stored in a transaction of their own.
///
/// An example of its use:
///
///     std::vector<pstore::gsl::czstring> strings{"foo", "bar"};
///     std::unordered_map<std::string, string_address> indir_strings;
///     add_export_strings (export_db_, std::begin (strings), std::end (strings),
///                         std::inserter (indir_strings, std::end (indir_strings)));
///
/// \param db  The database in which the strings will be written.
/// \param first  The first of the range of strings to be stored.
/// \param last  The end of the range of strings to be stored.
/// \param out  An OutputIterator which will store a pair<czstring, string_address>
template <typename InputIterator, typename OutputIterator>
void add_export_strings (pstore::database & db, InputIterator first, InputIterator last,
                         OutputIterator out) {
    mock_mutex mutex;
    auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
    auto const name_index = pstore::index::get_index<pstore::trailer::indices::name> (db);

    std::vector<pstore::raw_sstring_view> views;
    std::transform (first, last, std::back_inserter (views),
                    [] (pstore::gsl::czstring str) { return pstore::make_sstring_view (str); });

    using string_address = pstore::typed_address<pstore::indirect_string>;
    pstore::indirect_string_adder adder;
    std::transform (std::begin (views), std::end (views), out,
                    [&] (pstore::raw_sstring_view const & view) {
                        std::pair<pstore::index::name_index::iterator, bool> const res1 =
                            adder.add (transaction, name_index, &view);
                        return std::make_pair (view.to_string (),
                                               string_address::make (res1.first.get_address ()));
                    });

    adder.flush (transaction);
    transaction.commit ();
}

#endif // PSTORE_UNITTESTS_EXCHANGE_ADD_EXPORT_STRINGS_HPP
