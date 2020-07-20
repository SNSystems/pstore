//*  _                            _                                      *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __   __ _ _ __ ___   ___  ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '_ \ / _` | '_ ` _ \ / _ \/ __| *
//* | | | | | | | |_) | (_) | |  | |_  | | | | (_| | | | | | |  __/\__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_| |_|\__,_|_| |_| |_|\___||___/ *
//*             |_|                                                      *
//===- lib/exchange/import_names.cpp --------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
#include "pstore/exchange/import_names.hpp"

#include "pstore/core/indirect_string.hpp"
#include "pstore/exchange/import_error.hpp"

namespace pstore {
    namespace exchange {

        // add string
        // ~~~~~~~~~~
        std::error_code names::add_string (transaction_pointer transaction,
                                           std::string const & str) {
            strings_.push_back (str);
            std::string const & x = strings_.back ();

            views_.emplace_back (make_sstring_view (x));
            auto & s = views_.back ();

            std::shared_ptr<index::name_index> names_index =
                index::get_index<trailer::indices::name> (transaction->db ());
            std::pair<index::name_index::iterator, bool> const res =
                adder_.add (*transaction, names_index, &s);
            if (!res.second) {
                return {import_error::duplicate_name};
            }

            lookup_[lookup_.size ()] =
                typed_address<indirect_string>::make (res.first.get_address ());
            return {};
        }

        // flush
        // ~~~~~
        void names::flush (transaction_pointer transaction) { adder_.flush (*transaction); }

        // lookup
        // ~~~~~~
        error_or<typed_address<indirect_string>> names::lookup (std::uint64_t const index) const {
            using result_type = error_or<typed_address<indirect_string>>;

            auto const pos = lookup_.find (index);
            return pos != std::end (lookup_) ? result_type{pos->second}
                                             : result_type{import_error::no_such_name};
        }

    } // end namespace exchange
} // namespace pstore
