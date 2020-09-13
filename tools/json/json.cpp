//*    _                  *
//*   (_)___  ___  _ __   *
//*   | / __|/ _ \| '_ \  *
//*   | \__ \ (_) | | | | *
//*  _/ |___/\___/|_| |_| *
//* |__/                  *
//===- tools/json/json.cpp ------------------------------------------------===//
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
#include <array>
#include <fstream>
#include <iostream>

#include "pstore/cmd_util/tchar.hpp"
#include "pstore/dump/value.hpp"
#include "pstore/json/json.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp"

using pstore::cmd_util::error_stream;
using pstore::cmd_util::out_stream;

namespace {

    class yaml_output {
    public:
        using result_type = std::shared_ptr<pstore::dump::value>;

        std::error_code null_value ();
        std::error_code boolean_value (bool v);
        std::error_code string_value (std::string const & s);
        std::error_code int64_value (std::int64_t v);
        std::error_code uint64_value (std::uint64_t v);
        std::error_code double_value (double v);

        std::error_code begin_array ();
        std::error_code end_array ();

        std::error_code begin_object ();
        std::error_code key (std::string const & s);
        std::error_code end_object ();

        result_type result () const {
            assert (out_.size () == 1U);
            return out_.top ();
        }

    private:
        std::stack<result_type> out_;
    };

    std::error_code yaml_output::null_value () {
        out_.emplace (new pstore::dump::null ());
        return {};
    }

    std::error_code yaml_output::boolean_value (bool v) {
        out_.emplace (new pstore::dump::boolean (v));
        return {};
    }

    std::error_code yaml_output::string_value (std::string const & s) {
        out_.emplace (new pstore::dump::string (s));
        return {};
    }

    std::error_code yaml_output::int64_value (std::int64_t v) {
        out_.emplace (pstore::dump::make_number (v));
        return {};
    }

    std::error_code yaml_output::uint64_value (std::uint64_t v) {
        out_.emplace (pstore::dump::make_number (v));
        return {};
    }

    std::error_code yaml_output::double_value (double v) {
        out_.emplace (pstore::dump::make_number (v));
        return {};
    }

    std::error_code yaml_output::begin_array () {
        out_.emplace (nullptr);
        return {};
    }

    std::error_code yaml_output::end_array () {
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
        return {};
    }

    std::error_code yaml_output::begin_object () {
        out_.emplace (nullptr);
        return {};
    }

    std::error_code yaml_output::key (std::string const & s) {
        string_value (s);
        return {};
    }

    std::error_code yaml_output::end_object () {
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
        return {};
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
            pstore::json::coord const position = p.coordinate ();
            std::cerr << "Parse error: " << p.last_error ().message () << " (Line " << position.row
                      << ", column " << position.column << ")\n";
            return EXIT_FAILURE;
        }

        auto obj = p.callbacks ().result ();
        out_stream << NATIVE_TEXT ("\n----\n") << *obj << NATIVE_TEXT ('\n');
        return EXIT_SUCCESS;
    }

} // end anonymous namespace

#ifdef _WIN32
int _tmain (int argc, TCHAR const * argv[]) {
#else
int main (int argc, char const * argv[]) {
#endif
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
    PSTORE_CATCH (std::exception const & ex, { // clang-format on
        error_stream << NATIVE_TEXT ("Error: ") << pstore::utf::to_native_string (ex.what ())
                     << NATIVE_TEXT ('\n');
        exit_code = EXIT_FAILURE;
    })
    // clang-format off
    PSTORE_CATCH (..., { // clang-format on
        error_stream << NATIVE_TEXT ("Unknown exception.\n");
        exit_code = EXIT_FAILURE;
    })
    return exit_code;
}
