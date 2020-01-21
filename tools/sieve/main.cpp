//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===- tools/sieve/main.cpp -----------------------------------------------===//
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

#include <cassert>
#include <cstdint>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// pstore includes
#include "pstore/support/error.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/quoted.hpp"
#include "pstore/support/utf.hpp"

// Local includes
#include "switches.hpp"
#include "write_output.hpp"

namespace {

    template <typename T, typename R>
    struct check_range {
        void operator() (R value) const {
            (void) value;
            assert (value <= std::numeric_limits<T>::max ());
        }
    };
    template <typename T>
    struct check_range<T, T> {
        void operator() (T) const {}
    };


    template <typename IntType>
    std::vector<IntType> sieve (std::uint64_t top_value) {
        check_range<IntType, decltype (top_value)>{}(top_value);

        std::vector<IntType> result;
        result.push_back (1);
        result.push_back (2);

        std::vector<bool> is_prime ((top_value + 1) / 2 /*count*/, 1 /*value*/);
        for (auto ctr = 3UL; ctr <= top_value; ctr += 2) {
            if (is_prime[ctr / 2]) {
                result.push_back (static_cast<IntType> (ctr));

                for (auto multiple = ctr * ctr; multiple <= top_value; multiple += 2 * ctr) {
                    is_prime[multiple / 2] = 0;
                }
            }
        }
        return result;
    }

    std::function<std::ostream &()> open_output_file (std::string const & path) {
        if (path == "-") {
            return [] () { return std::ref (std::cout); };
        }

        auto file = std::make_shared<std::ofstream> (
            path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
        if (!file->is_open ()) {
            std::ostringstream str;
            str << "Could not open " << pstore::quoted (path);
            pstore::raise (std::errc::no_such_file_or_directory, str.str ());
        }
        return [file] () { return std::ref (*file); };
    }

} // anonymous namespace


#if defined(_WIN32)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    PSTORE_TRY {
        user_options const opt = user_options::get (argc, argv);

        auto out = open_output_file (opt.output);
        if (opt.maximum <= std::numeric_limits<std::uint16_t>::max ()) {
            write_output (sieve<std::uint16_t> (opt.maximum), opt.endianness, out);
        } else if (opt.maximum <= std::numeric_limits<std::uint32_t>::max ()) {
            write_output (sieve<std::uint32_t> (opt.maximum), opt.endianness, out);
        } else {
            write_output (sieve<std::uint64_t> (opt.maximum), opt.endianness, out);
        }
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, {
        pstore::cmd_util::error_stream << NATIVE_TEXT ("An error occurred: ")
                                       << pstore::utf::to_native_string (ex.what ())
                                       << std::endl;
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        pstore::cmd_util::error_stream << NATIVE_TEXT ("Unknown exception") << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format on

    return exit_code;
}
