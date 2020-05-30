//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//===- tools/import/import.cpp --------------------------------------------===//
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
#include "pstore/json/json.hpp"

#include "import_error.hpp"
#include "import_terminals.hpp"
#include "import_transaction.hpp"

using namespace pstore;

//*               _         _     _        _    *
//*  _ _ ___  ___| |_   ___| |__ (_)___ __| |_  *
//* | '_/ _ \/ _ \  _| / _ \ '_ \| / -_) _|  _| *
//* |_| \___/\___/\__| \___/_.__// \___\__|\__| *
//*                            |__/             *
class root_object final : public state {
public:
    root_object (parse_stack_pointer stack, not_null<database *> db)
            : state (stack)
            , db_{db} {}
    gsl::czstring name () const noexcept;
    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;

private:
    not_null<database *> db_;

    enum { version, transactions };
    std::bitset<transactions + 1> seen_;
    std::uint64_t version_ = 0;
};

// name
// ~~~~
gsl::czstring root_object::name () const noexcept {
    return "root object";
}

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
        return push<transaction_array> (db_);
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

//*               _    *
//*  _ _ ___  ___| |_  *
//* | '_/ _ \/ _ \  _| *
//* |_| \___/\___/\__| *
//*                    *
class root final : public state {
public:
    root (parse_stack_pointer stack, not_null<database *> db)
            : state (stack)
            , db_{db} {}
    gsl::czstring name () const noexcept override;
    std::error_code begin_object () override;

private:
    not_null<database *> db_;
};

// name
// ~~~~
gsl::czstring root::name () const noexcept {
    return "root";
}

// begin object
// ~~~~~~~~~~~~
std::error_code root::begin_object () {
    return push<root_object> (db_);
}



int main (int argc, char * argv[]) {
    try {
        pstore::database db{argv[1], pstore::database::access_mode::writable};
        FILE * infile = argc == 3 ? std::fopen (argv[2], "r") : stdin;
        if (infile == nullptr) {
            std::perror ("File opening failed");
            return EXIT_FAILURE;
        }

        auto parser = json::make_parser (callbacks::make<root> (&db));
        using parser_type = decltype (parser);
        for (int ch = std::getc (infile); ch != EOF; ch = std::getc (infile)) {
            auto const c = static_cast<char> (ch);
            // std::cout << c;
            parser.input (&c, &c + 1);
            if (parser.has_error ()) {
                std::error_code const erc = parser.last_error ();
                // raise (erc);
                std::cerr << "Value: " << erc.value () << '\n';
                std::cerr << "Error: " << erc.message () << '\n';
                std::cout << "Row " << std::get<parser_type::row_index> (parser.coordinate ())
                          << ", column "
                          << std::get<parser_type::column_index> (parser.coordinate ()) << '\n';
                break;
            }
        }
        parser.eof ();

        if (std::feof (infile)) {
            std::cout << "\n End of file reached.";
        } else {
            std::cout << "\n Something went wrong.";
        }
        if (infile != nullptr && infile != stdin) {
            std::fclose (infile);
            infile = nullptr;
        }
    } catch (std::exception const & ex) {
        std::cerr << "Error: " << ex.what () << '\n';
    } catch (...) {
        std::cerr << "Error: an unknown error occurred\n";
    }
}
