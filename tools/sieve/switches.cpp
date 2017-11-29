//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/sieve/switches.cpp -------------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
/// \file switches.cpp

#include "switches.hpp"

#if !PSTORE_IS_INSIDE_LLVM
// Standard library includes
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

// 3rd party
#include "optionparser.h"

// PStore includes
#include "pstore_support/utf.hpp"


namespace {
    static std::map<std::string, switches::endian> const endians{
        {"native", switches::endian::native},
        {"big", switches::endian::big},
        {"little", switches::endian::little},
    };


    unsigned long to_ulong (char const * str) {
        if (str == nullptr || str[0] == '\0') {
            throw switches::bad_number ();
        }

        char * endptr = nullptr;
        long v = std::strtol (str, &endptr, 10);
        if (endptr == nullptr || endptr == str || *endptr != '\0' || v < 0) {
            throw switches::bad_number ();
        }
        return static_cast<unsigned long> (v);
    }

    switches::endian to_endian (char const * str) {
        if (str != nullptr) {
            std::string s{str};
            // Convert the string to lower case.
            std::transform (std::begin (s), std::end (s), std::begin (s),
                            [](char c) { return static_cast<char> (std::tolower (c)); });

            auto it = endians.find (s);
            if (it != endians.end ()) {
                return it->second;
            }
        }
        throw switches::bad_endian ();
    }
}


namespace {
#if defined(_WIN32) && defined(_UNICODE)
    auto & error_stream = std::wcerr;
    auto & out_stream = std::wcout;
#else
    auto & error_stream = std::cerr;
    auto & out_stream = std::cout;
#endif
}


namespace {
    constexpr unsigned long default_maximum = 100;
    struct maximum_help_text {
        maximum_help_text ();
        pstore::utf::native_string text;
    };

    maximum_help_text::maximum_help_text () {
        pstore::utf::native_ostringstream str;
        str << NATIVE_TEXT ("  --maximum, -m  \tThe maximum prime value. (Default: ")
            << default_maximum << NATIVE_TEXT (")");
        text = str.str ();
    }

    static maximum_help_text const max_help;

    constexpr auto default_endian = switches::endian::native;
    class endian_help {
    public:
        endian_help ();
        pstore::utf::native_string text;
        pstore::utf::native_string long_text;
    };

    endian_help::endian_help () {
        {
            pstore::utf::native_ostringstream str;
            str << NATIVE_TEXT ("May be any of: ");
            TCHAR const * separator = NATIVE_TEXT ("");
            for (auto const & e : endians) {
                str << separator << pstore::utf::to_native_string (e.first);
                separator = NATIVE_TEXT (", ");
            }
            text = str.str ();
        }
        {
            pstore::utf::native_ostringstream long_str;
            long_str << NATIVE_TEXT ("  --endian, -e  \tThe endian-ness of the output data. ")
                     << text << NATIVE_TEXT (" (Default: native)");
            long_text = long_str.str ();
        }
    }

    static endian_help const endian_help;
}


namespace switches {

    bad_number::bad_number ()
            : std::runtime_error ("Bad number") {}

    bad_endian::bad_endian ()
            : std::runtime_error ("Bad value for endian") {}

    parse_failure::parse_failure ()
            : std::runtime_error ("Command line options were not parsed successfully") {}
}


namespace {

    enum option_index {
        unknown_opt,
        endian_opt,
        help_opt,
        output_opt,
        maximum_opt,
    };

    option::ArgStatus output (option::Option const & option, bool msg) {
        if (option.arg != nullptr && option.arg[0] != '\0') {
            return option::ARG_OK;
        }
        if (msg) {
            error_stream << NATIVE_TEXT ("Option '") << option.name
                         << NATIVE_TEXT ("' requires a file path\n");
        }
        return option::ARG_ILLEGAL;
    }

    option::ArgStatus endian (option::Option const & option, bool msg) {
        bool ok = true;
        try {
            to_endian (pstore::utf::from_native_string (option.arg).c_str ());
        } catch (switches::bad_endian const &) {
            ok = false;
        }

        if (!ok) {
            if (msg) {
                error_stream << NATIVE_TEXT ("Option '") << option.name
                             << NATIVE_TEXT ("' was not valid. ") << endian_help.text
                             << NATIVE_TEXT ("\n");
            }
            return option::ARG_ILLEGAL;
        }
        return option::ARG_OK;
    }

    option::ArgStatus maximum (option::Option const & option, bool msg) {
        bool ok = true;
        try {
            to_ulong (pstore::utf::from_native_string (option.arg).c_str ());
        } catch (switches::bad_number const &) {
            ok = false;
        }

        if (!ok) {
            if (msg) {
                error_stream << NATIVE_TEXT ("Option '") << option.name
                             << NATIVE_TEXT ("' not recognized. ") << endian_help.text
                             << NATIVE_TEXT ("\n");
            }
            return option::ARG_ILLEGAL;
        }
        return option::ARG_OK;
    }


    option::Descriptor const usage[] = {
        {unknown_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT (""), option::Arg::None,
         NATIVE_TEXT ("Usage: sieve [options]\n\nOptions:")},
        {help_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT ("help"), option::Arg::None,
         NATIVE_TEXT ("  --help  \tPrint usage and exit.")},
        {endian_opt, 0, NATIVE_TEXT ("e"), NATIVE_TEXT ("endian"), endian,
         endian_help.long_text.c_str ()},
        {output_opt, 0, NATIVE_TEXT ("o"), NATIVE_TEXT ("output"), output,
         NATIVE_TEXT ("  --output, -o  \tOutput file name. (Default: standard out)")},
        {maximum_opt, 0, NATIVE_TEXT ("m"), NATIVE_TEXT ("maximum"), maximum,
         max_help.text.c_str ()},

        {unknown_opt, 0, NATIVE_TEXT (""), NATIVE_TEXT (""), option::Arg::None,
         NATIVE_TEXT (
             "\nExamples:\n"
             "  sieve               \t Writes the first 100 primes to standard out.\n"
             "  sieve -o foo -m 500 \t Writes the first 500 primes to a file named 'foo'.\n")},
        {0, 0, 0, 0, 0, 0},
    };
}

namespace switches {

    user_options user_options::get (int argc, TCHAR * argv[]) {
        // Skip program name argv[0] if present
        argc -= (argc > 0);
        argv += (argc > 0);

        option::Stats stats (usage, argc, argv);
        std::vector<option::Option> options (stats.options_max);
        std::vector<option::Option> buffer (stats.buffer_max);
        option::Parser parse (usage, argc, argv, options.data (), buffer.data ());
        if (parse.error ()) {
            throw parse_failure ();
        }

        if (options[help_opt] || parse.nonOptionsCount () != 0) {
            option::printUsage (out_stream, usage);
            throw parse_failure ();
        }

        std::shared_ptr<std::string> output_path;
        if (auto const & output = options[output_opt]) {
            TCHAR const * path = output.arg;
            assert (path != nullptr && path[0] != '\0');
            output_path = std::make_shared<std::string> (pstore::utf::from_native_string (path));
        }

        auto output_endian = default_endian;
        if (auto const & opt = options[endian_opt]) {
            output_endian = to_endian (pstore::utf::from_native_string (opt.arg).c_str ());
        }

        auto maximum = default_maximum;
        if (auto const & opt = options[maximum_opt]) {
            maximum = to_ulong (pstore::utf::from_native_string (opt.arg).c_str ());
        }

        return {output_path, output_endian, maximum};
    }
}
#endif // !PSTORE_IS_INSIDE_LLVM
// eof: tools/sieve/switches.cpp
