//*                          _    *
//*   __ _ ___ ___  ___ _ __| |_  *
//*  / _` / __/ __|/ _ \ '__| __| *
//* | (_| \__ \__ \  __/ |  | |_  *
//*  \__,_|___/___/\___|_|   \__| *
//*                               *
//===- lib/support/assert.cpp ---------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file assert.hpp
/// \brief Defines assert_failed() which is required by PSTORE_ASSERT.

#include "pstore/support/assert.hpp"

#include <array>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#    include <tchar.h>
#endif // _WIN32

#include "pstore/support/gsl.hpp"

#include "backtrace.hpp"

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
