//*  _                            _                     _    *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __ ___   ___ | |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '__/ _ \ / _ \| __| *
//* | | | | | | | |_) | (_) | |  | |_  | | | (_) | (_) | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_|  \___/ \___/ \__| *
//*             |_|                                          *
//===- lib/exchange/import_root.cpp ---------------------------------------===//
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
#include "pstore/exchange/import_root.hpp"

#include <bitset>

#include "pstore/core/database.hpp"
#include "pstore/exchange/import_error.hpp"
#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_non_terminals.hpp"
#include "pstore/exchange/import_terminals.hpp"
#include "pstore/exchange/import_transaction.hpp"

using namespace pstore;
using namespace exchange;

namespace {

    //*               _         _     _        _    *
    //*  _ _ ___  ___| |_   ___| |__ (_)___ __| |_  *
    //* | '_/ _ \/ _ \  _| / _ \ '_ \| / -_) _|  _| *
    //* |_| \___/\___/\__| \___/_.__// \___\__|\__| *
    //*                            |__/             *
    class root_object final : public rule {
    public:
        root_object (parse_stack_pointer stack, not_null<database *> db)
                : rule (stack)
                , db_{db} {}
        gsl::czstring name () const noexcept override;
        std::error_code key (std::string const & k) override;
        std::error_code end_object () override;

    private:
        not_null<database *> db_;
        names names_;

        enum { version, transactions };
        std::bitset<transactions + 1> seen_;
        std::uint64_t version_ = 0;
    };

    // name
    // ~~~~
    gsl::czstring root_object::name () const noexcept { return "root object"; }

    // key
    // ~~~
    std::error_code root_object::key (std::string const & k) {
        // TODO: check that 'version' is the first key that we see.
        if (k == "version") {
            seen_[version] = true;
            return push<uint64_rule> (&version_);
        }
        if (k == "transactions") {
            seen_[transactions] = true;
            return push<transaction_array> (db_, &names_);
        }
        return import_error::unrecognized_root_key;
    }

    // end object
    // ~~~~~~~~~~
    std::error_code root_object::end_object () {
        if (!seen_.all ()) {
            return import_error::root_object_was_incomplete;
        }
        return {};
    }

} // end anonymous namespace

namespace pstore {
    namespace exchange {

        //*               _    *
        //*  _ _ ___  ___| |_  *
        //* | '_/ _ \/ _ \  _| *
        //* |_| \___/\___/\__| *
        //*                    *
        // name
        // ~~~~
        gsl::czstring root::name () const noexcept { return "root"; }

        // begin object
        // ~~~~~~~~~~~~
        std::error_code root::begin_object () { return push<root_object> (db_); }


        // create import parser
        // ~~~~~~~~~~~~~~~~~~~~
        json::parser<callbacks> create_import_parser (database & db) {
            return json::make_parser (callbacks::make<root> (&db));
        }

    } // end namespace exchange
} // end namespace pstore
