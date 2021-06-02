//===- lib/exchange/import_root.cpp ---------------------------------------===//
//*  _                            _                     _    *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __ ___   ___ | |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '__/ _ \ / _ \| __| *
//* | | | | | | | |_) | (_) | |  | |_  | | | (_) | (_) | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_|  \___/ \___/ \__| *
//*             |_|                                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
    class root_object final : public pstore::exchange::import_ns::rule {
    public:
        explicit root_object (
            pstore::gsl::not_null<pstore::exchange::import_ns::context *> const ctxt)
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
        pstore::exchange::import_ns::string_mapping names_;
        pstore::exchange::import_ns::string_mapping paths_;

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
        using namespace pstore::exchange::import_ns;

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
            return push<transaction_array<pstore::transaction_lock>> (&names_, &paths_);
        }
        return error::unrecognized_root_key;
    }

    // end object
    // ~~~~~~~~~~
    std::error_code root_object::end_object () {
        using namespace pstore::exchange::import_ns;

        if (!seen_.all ()) {
            return error::root_object_was_incomplete;
        }

        this->get_context ()->db->set_id (id_);
        return {};
    }

} // end anonymous namespace

namespace pstore {
    namespace exchange {
        namespace import_ns {

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
                return json::make_parser (callbacks::make<root> (&db), json::extensions::all);
            }

        } // end namespace import_ns
    }     // end namespace exchange
} // end namespace pstore
