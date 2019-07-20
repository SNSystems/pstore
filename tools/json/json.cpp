//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===- tools/json/json.cpp ------------------------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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

#include "pstore/dump/value.hpp"
#include "pstore/json/json.hpp"
#include "pstore/support/portab.hpp"

namespace {

    class yaml_output {
    public:
        using result_type = std::shared_ptr<pstore::dump::value>;

        void string_value (std::string const & s) { out_.emplace (new pstore::dump::string (s)); }
        void integer_value (long v) { out_.emplace (pstore::dump::make_number (v)); }
        void float_value (double v) { out_.emplace (pstore::dump::make_number (v)); }
        void boolean_value (bool v) { out_.emplace (new pstore::dump::boolean (v)); }
        void null_value () { out_.emplace (new pstore::dump::null ()); }

        void begin_array () { out_.emplace (nullptr); }
        void end_array ();

        void begin_object () { out_.emplace (nullptr); }
        void end_object ();

        result_type result () const {
            assert (out_.size () == 1U);
            return out_.top ();
        }

    private:
        std::stack<result_type> out_;
    };

    void yaml_output::end_array () {
        pstore::dump::array::container content;
        for (;;) {
            auto v = out_.top ();
            out_.pop ();
            if (!v) {
                break;
            }
            content.push_back (std::move (v));
        }
        std::reverse (std::begin (content), std::end (content));
        out_.emplace (new pstore::dump::array (std::move (content)));
    }

    void yaml_output::end_object () {
        auto object = std::make_shared<pstore::dump::object> ();
        for (;;) {
            auto value = out_.top ();
            out_.pop ();
            if (!value) {
                break;
            }

            auto key = out_.top ();
            out_.pop ();
            assert (key);
            auto key_str = key->dynamic_cast_string ();
            assert (key_str);
            object->insert (key_str->get (), value);
        }
        out_.emplace (std::move (object));
    }

    template <typename IStream>
    int slurp (IStream & in) {
        std::array<char, 256> buffer{{0}};
        pstore::json::parser<yaml_output> p;

        while ((in.rdstate () &
                (std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit)) == 0 &&
               !p.has_error ()) {
            in.read (buffer.data (), buffer.size ());
            p.input (pstore::gsl::make_span (buffer.data (),
                                             std::max (in.gcount (), std::streamsize{0})));
        }

        p.eof ();

        if (in.bad ()) {
            std::cerr << "There was an I/O error while reading.\n";
            return EXIT_FAILURE;
        }

        auto err = p.last_error ();
        if (err) {
            std::tuple<unsigned, unsigned> const position = p.coordinate ();
            std::cerr << "Parse error: " << p.last_error ().message () << " (Line "
                      << std::get<1> (position) << ", column " << std::get<0> (position) << ")\n";
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
