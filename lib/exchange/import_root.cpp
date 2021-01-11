//*  _                            _                     _    *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __ ___   ___ | |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '__/ _ \ / _ \| __| *
//* | | | | | | | |_) | (_) | |  | |_  | | | (_) | (_) | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_|  \___/ \___/ \__| *
//*             |_|                                          *
//===- lib/exchange/import_root.cpp ---------------------------------------===//
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
#include "pstore/exchange/import_root.hpp"

#include <bitset>

#include "pstore/exchange/import_non_terminals.hpp"
#include "pstore/exchange/import_transaction.hpp"
#include "pstore/exchange/import_uuid.hpp"

namespace {

    //*               _         _     _        _    *
    //*  _ _ ___  ___| |_   ___| |__ (_)___ __| |_  *
    //* | '_/ _ \/ _ \  _| / _ \ '_ \| / -_) _|  _| *
    //* |_| \___/\___/\__| \___/_.__// \___\__|\__| *
    //*                            |__/             *
    class root_object final : public pstore::exchange::import::rule {
    public:
        explicit root_object (pstore::gsl::not_null<pstore::exchange::import::context *> const ctxt)
                : rule (ctxt) {}
        root_object (root_object const &) = delete;
        root_object (root_object &&) noexcept = delete;

        ~root_object () noexcept override = default;

        root_object & operator= (root_object const &) = delete;
        root_object & operator= (root_object &&) noexcept = delete;

        pstore::gsl::czstring name () const noexcept override;
        std::error_code key (std::string const & k) override;
        std::error_code end_object () override;

    private:
        pstore::exchange::import::name_mapping names_;

        enum { version, id, transactions };
        std::bitset<transactions + 1> seen_;
        std::uint64_t version_ = 0;
        pstore::uuid id_;
    };

    // name
    // ~~~~
    pstore::gsl::czstring root_object::name () const noexcept { return "root object"; }

    // key
    // ~~~
    std::error_code root_object::key (std::string const & k) {
        using namespace pstore::exchange::import;

        // TODO: check that 'version' is the first key that we see.
        if (k == "version") {
            seen_[version] = true;
            return push<uint64_rule> (&version_);
        }
        if (k == "id") {
            seen_[id] = true;
            return push<uuid_rule> (&id_);
        }
        if (k == "transactions") {
            seen_[transactions] = true;
            return push<transaction_array<pstore::transaction_lock>> (&names_);
        }
        return error::unrecognized_root_key;
    }

    // end object
    // ~~~~~~~~~~
    std::error_code root_object::end_object () {
        if (!seen_.all ()) {
            return pstore::exchange::import::error::root_object_was_incomplete;
        }
        return {};
    }

} // end anonymous namespace

namespace pstore {
    namespace exchange {
        namespace import {

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
            std::error_code root::begin_object () { return this->push<root_object> (); }


            // create parser
            // ~~~~~~~~~~~~~
            json::parser<callbacks> create_parser (database & db) {
                return json::make_parser (callbacks::make<root> (&db),
                                          json::parser_extensions::all);
            }

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore
