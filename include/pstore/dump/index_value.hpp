//*  _           _                        _             *
//* (_)_ __   __| | _____  __ __   ____ _| |_   _  ___  *
//* | | '_ \ / _` |/ _ \ \/ / \ \ / / _` | | | | |/ _ \ *
//* | | | | | (_| |  __/>  <   \ V / (_| | | |_| |  __/ *
//* |_|_| |_|\__,_|\___/_/\_\   \_/ \__,_|_|\__,_|\___| *
//*                                                     *
//===- include/pstore/dump/index_value.hpp --------------------------------===//
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
