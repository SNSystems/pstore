//*                          _    *
//*   __ _ ___ ___  ___ _ __| |_  *
//*  / _` / __/ __|/ _ \ '__| __| *
//* | (_| \__ \__ \  __/ |  | |_  *
//*  \__,_|___/___/\___|_|   \__| *
//*                               *
//===- lib/support/assert.cpp ---------------------------------------------===//
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
#include "pstore/support/assert.hpp"

#include <array>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#    include <tchar.h>
#endif // _WIN32

#include "pstore/support/gsl.hpp"

#include "backtrace.hpp"

#ifndef NDEBUG

namespace {

#    ifdef _WIN32
    auto & out_stream = std::wcerr;
#        define NATIVE_TEXT(str) _TEXT (str)
#    else
    auto & out_stream = std::cerr;
#        define NATIVE_TEXT(str) str
#    endif

} // end anonymous namespace

namespace pstore {

    void assert_failed (gsl::czstring str, gsl::czstring file, int line) {
        out_stream << NATIVE_TEXT ("Assert failed: (") << str << NATIVE_TEXT ("), file ") << file
                   << NATIVE_TEXT (", line ") << line << std::endl;
#    if PSTORE_HAVE_BACKTRACE
        std::array<void *, 64U> callstack;
        void ** const out = callstack.data ();
        int const frames = ::backtrace (out, static_cast<int> (callstack.size ()));

        auto const deleter = [] (void * p) {
            if (p != nullptr) {
                std::free (p);
            }
        };
        std::unique_ptr<gsl::zstring, decltype (deleter)> strs{::backtrace_symbols (out, frames),
                                                               deleter};
        auto begin = strs.get ();
        std::copy (begin, begin + frames,
                   std::ostream_iterator<gsl::czstring> (out_stream, NATIVE_TEXT ("\n")));
#    endif // PSTORE_HAVE_BACKTRACE
        std::abort ();
    }

} // end namespace pstore

#endif // NDEBUG
