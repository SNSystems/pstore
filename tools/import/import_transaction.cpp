//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//*  _                                  _   _              *
//* | |_ _ __ __ _ _ __  ___  __ _  ___| |_(_) ___  _ __   *
//* | __| '__/ _` | '_ \/ __|/ _` |/ __| __| |/ _ \| '_ \  *
//* | |_| | | (_| | | | \__ \ (_| | (__| |_| | (_) | | | | *
//*  \__|_|  \__,_|_| |_|___/\__,_|\___|\__|_|\___/|_| |_| *
//*                                                        *
//===- tools/import/import_transaction.cpp --------------------------------===//
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
#include "import_transaction.hpp"

#include "import_compilations.hpp"
#include "import_debug_line_header.hpp"
#include "import_error.hpp"
#include "import_fragment.hpp"
#include "import_names.hpp"
#include "import_non_terminals.hpp"

using namespace pstore;

namespace {

    class transaction_contents final : public rule {
    public:
        transaction_contents (parse_stack_pointer stack, not_null<database *> db)
                : rule (stack)
                , transaction_{begin (*db)}
                , names_{&transaction_} {}

    private:
        std::error_code key (std::string const & s) override;
        std::error_code end_object () override;
        gsl::czstring name () const noexcept override;

        transaction_type transaction_;
        names names_;
    };

    gsl::czstring transaction_contents::name () const noexcept { return "transaction contents"; }
    std::error_code transaction_contents::key (std::string const & s) {
        // TODO: check that "names" is the first key that we see.
        if (s == "names") {
            return push_array_rule<names_array_members> (this, &names_);
        }
        if (s == "debugline") {
            return push_object_rule<debug_line_index> (this, &transaction_);
        }
        if (s == "fragments") {
            return push_object_rule<fragment_index> (this, &transaction_);
        }
        if (s == "compilations") {
            return push_object_rule<compilations_index> (this, &transaction_, &names_);
        }
        return import_error::unknown_transaction_object_key;
    }

    std::error_code transaction_contents::end_object () {
        names_.flush ();
        transaction_.commit ();
        return pop ();
    }


    class transaction_object final : public rule {
    public:
        transaction_object (parse_stack_pointer s, not_null<database *> db)
                : rule (s)
                , db_{db} {}
        gsl::czstring name () const noexcept override { return "transaction_object"; }
        std::error_code begin_object () override { return push<transaction_contents> (db_); }
        std::error_code end_array () override { return pop (); }

    private:
        not_null<database *> db_;
    };

} // end anonymous namespace

//*  _                             _   _                                    *
//* | |_ _ _ __ _ _ _  ___ __ _ __| |_(_)___ _ _    __ _ _ _ _ _ __ _ _  _  *
//* |  _| '_/ _` | ' \(_-</ _` / _|  _| / _ \ ' \  / _` | '_| '_/ _` | || | *
//*  \__|_| \__,_|_||_/__/\__,_\__|\__|_\___/_||_| \__,_|_| |_| \__,_|\_, | *
//*                                                                   |__/  *
// (ctor)
// ~~~~~~
transaction_array::transaction_array (parse_stack_pointer s, not_null<database *> db)
        : rule (s)
        , db_{db} {}

// name
// ~~~~
gsl::czstring transaction_array::name () const noexcept {
    return "transaction array";
}

// begin_array
// ~~~~~~~~~~~
std::error_code transaction_array::begin_array () {
    return this->replace_top<transaction_object> (db_);
}
