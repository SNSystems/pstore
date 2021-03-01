//===- include/pstore/dump/index_value.hpp ----------------*- mode: C++ -*-===//
//*  _           _                        _             *
//* (_)_ __   __| | _____  __ __   ____ _| |_   _  ___  *
//* | | '_ \ / _` |/ _ \ \/ / \ \ / / _` | | | | |/ _ \ *
//* | | | | | (_| |  __/>  <   \ V / (_| | | |_| |  __/ *
//* |_|_| |_|\__,_|\___/_/\_\   \_/ \__,_|_|\__,_|\___| *
//*                                                     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_DUMP_INDEX_VALUE_HPP
#define PSTORE_DUMP_INDEX_VALUE_HPP

#include "pstore/core/index_types.hpp"
#include "pstore/dump/mcrepo_value.hpp"

namespace pstore {
    namespace dump {

        template <typename trailer::indices Index, typename MakeValueFn>
        value_ptr make_index (database const & db, MakeValueFn mk) {
            using return_type = typename index::enum_to_index<Index>::type const;
            using value_type = typename return_type::value_type;
            array::container members;
            if (std::shared_ptr<return_type> const index =
                    index::get_index<Index> (db, false /* create */)) {
                std::for_each (
                    index->begin (db), index->end (db),
                    [&members, mk] (value_type const & v) { members.emplace_back (mk (v)); });
            }
            return make_value (std::move (members));
        }
    } // namespace dump
} // namespace pstore

#endif // PSTORE_DUMP_INDEX_VALUE_HPP
