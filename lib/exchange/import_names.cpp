//*  _                            _                                      *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __   __ _ _ __ ___   ___  ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '_ \ / _` | '_ ` _ \ / _ \/ __| *
//* | | | | | | | |_) | (_) | |  | |_  | | | | (_| | | | | | |  __/\__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_| |_|\__,_|_| |_| |_|\___||___/ *
//*             |_|                                                      *
//===- lib/exchange/import_names.cpp --------------------------------------===//
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
/// \file import_names.cpp
/// \brief Implements the class which maps from string indexes to their store address.

#include "pstore/exchange/import_names.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            // lookup
            // ~~~~~~
            error_or<typed_address<indirect_string>>
            name_mapping::lookup (std::uint64_t const index) const {
                using result_type = error_or<typed_address<indirect_string>>;

                auto const pos = lookup_.find (index);
                return pos != std::end (lookup_) ? result_type{pos->second}
                                                 : result_type{error::no_such_name};
            }

            // add string
            // ~~~~~~~~~~
            std::error_code
            name_mapping::add_string (not_null<transaction_base *> const transaction,
                                      std::string const & str) {
                strings_.push_back (str);
                std::string const & x = strings_.back ();

                views_.emplace_back (make_sstring_view (x));
                auto & s = views_.back ();

                std::shared_ptr<index::name_index> const names_index =
                    index::get_index<trailer::indices::name> (transaction->db ());
                std::pair<index::name_index::iterator, bool> const add_res =
                    adder_.add (*transaction, names_index, &s);
                if (!add_res.second) {
                    return error::duplicate_name;
                }

                lookup_.emplace (lookup_.size (), typed_address<indirect_string>::make (
                                                      add_res.first.get_address ()));
                return {};
            }

            // flush
            // ~~~~~
            void name_mapping::flush (not_null<transaction_base *> const transaction) {
                adder_.flush (*transaction);
            }

        } // end namespace import
    }     // end namespace exchange
} // namespace pstore
