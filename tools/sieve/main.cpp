//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===- tools/sieve/main.cpp -----------------------------------------------===//
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

#include <cassert>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <tchar.h>
#endif

// Local includes
#include "switches.hpp"
#include "write_output.hpp"

#ifdef PSTORE_IS_INSIDE_LLVM
#define TRY
#define CATCH(ex,body)
#else
#define TRY try
#define CATCH(ex, body) catch (ex) body
#endif


namespace {

    template <typename IntType>
    std::vector<IntType> sieve (unsigned long top_value) {
        assert (top_value <= std::numeric_limits <IntType>::max ());

        std::vector<IntType> result;
        result.push_back (1);
        result.push_back (2);

        std::vector <bool> is_prime ((top_value + 1) / 2 /*count*/, 1/*value*/);
        for (auto ctr = 3UL; ctr <= top_value; ctr += 2) {
            if (is_prime [ctr / 2]) {
                result.push_back (static_cast <IntType> (ctr));

                for (auto multiple = ctr * ctr; multiple <= top_value; multiple += 2 * ctr) {
                    is_prime [multiple / 2] = 0;
                }
            }
        }
        return result;
    }

} // anonymous namespace


#if defined(_WIN32) && !defined(PSTORE_IS_INSIDE_LLVM)
int _tmain (int argc, TCHAR * argv []) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    TRY {
        switches::user_options const opt = switches::user_options::get (argc, argv);

        std::unique_ptr<std::ostream> out_ptr;
        std::ostream * out = &std::cout;
        if (opt.output != nullptr && *(opt.output) != "-") {
            std::string const & path = *opt.output;
            auto constexpr mode = std::ios_base::binary | std::ios_base::out | std::ios_base::trunc;
            auto file = std::unique_ptr<std::ofstream> (new std::ofstream (path, mode));
            if (!file->is_open ()) {
                std::ostringstream str;
                str << "Could not open '" << path << "'";
                return EXIT_FAILURE;
            }
            out_ptr.reset (file.release ());
            out = out_ptr.get ();
        }

        if (opt.maximum <= std::numeric_limits<std::uint16_t>::max ()) {
            write_output (sieve<std::uint16_t> (opt.maximum), opt.endianness, out);
        } else if (opt.maximum <= std::numeric_limits<std::uint32_t>::max ()) {
            write_output (sieve<std::uint32_t> (opt.maximum), opt.endianness, out);
        } else {
            write_output (sieve<std::uint64_t> (opt.maximum), opt.endianness, out);
        }
    } CATCH (switches::parse_failure const &, {
        exit_code = EXIT_FAILURE;
    }) CATCH (std::exception const & ex, {
        std::cerr << "An error occurred: " << ex.what () << std::endl;
        exit_code = EXIT_FAILURE;
    }) CATCH (..., {
        std::cerr << "Unknown exception" << std::endl;
        exit_code = EXIT_FAILURE;
    })

    return exit_code;
}
// eof: tools/sieve/main.cpp
