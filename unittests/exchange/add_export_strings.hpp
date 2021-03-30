//===- unittests/exchange/add_export_strings.hpp ----------*- mode: C++ -*-===//
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
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
///     using index = pstore::trailer::indices::name;
///     add_export_strings<index> (export_db_,
///                                std::begin (strings), std::end (strings),
///                                std::inserter (indir_strings, std::end (indir_strings)));
///
/// \tparam Index  Tge index in which the strings are held.
/// \param db  The database in which the strings will be written.
/// \param first  The first of the range of strings to be stored.
/// \param last  The end of the range of strings to be stored.
/// \param out  An OutputIterator which will store a pair<czstring, string_address>
template <pstore::trailer::indices Index, typename InputIterator, typename OutputIterator>
void add_export_strings (pstore::database & db, InputIterator first, InputIterator last,
                         OutputIterator out) {
    mock_mutex mutex;
    auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
    auto const name_index = pstore::index::get_index<Index> (db);

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
