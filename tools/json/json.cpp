//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===- tools/json/json.cpp ------------------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
#include <array>
#include <fstream>
#include <iostream>

#include "pstore/json/dom_types.hpp"
#include "pstore/json/json.hpp"
#include "pstore/json/utf.hpp"
#include "pstore/support/portab.hpp"

namespace {

    template <typename IStream>
    int slurp (IStream & in) {
        std::array<char, 256> buffer{{0}};
        pstore::json::parser<pstore::json::yaml_output> p;

        while ((in.rdstate () &
                (std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit)) == 0 &&
               !p.has_error ()) {
            in.read (buffer.data (), buffer.size ());
            p.parse (pstore::gsl::make_span (buffer.data (),
                                             std::max (in.gcount (), std::streamsize{0})));
        }

        p.eof ();

        if (in.bad ()) {
            std::cerr << "There was an I/O error while reading.\n";
            return EXIT_FAILURE;
        }

        auto err = p.last_error ();
        if (err) {
            std::cerr << "Parse error: " << p.last_error ().message () << '\n';
            return EXIT_FAILURE;
        }

        auto obj = p.callbacks ().result ();
        std::cout << "\n----\n" << *obj << '\n';
        return EXIT_SUCCESS;
    }

} // end anonymous namespace

int main (int argc, const char * argv[]) {
    int exit_code = EXIT_SUCCESS;
    PSTORE_TRY {
        if (argc < 2) {
            exit_code = slurp (std::cin);
        } else {
            std::ifstream input (argv[1]);
            exit_code = slurp (input);
        }
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, {
        std::cerr << "Error: " << ex.what () << '\n';
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        std::cerr << "Unknown exception.\n";
        exit_code = EXIT_FAILURE;
    })
    // clang-format on
    return exit_code;
}
// eof: tools/json/json.cpp
