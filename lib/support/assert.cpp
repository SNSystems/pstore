//===- lib/support/assert.cpp ---------------------------------------------===//
//*                          _    *
//*   __ _ ___ ___  ___ _ __| |_  *
//*  / _` / __/ __|/ _ \ '__| __| *
//* | (_| \__ \__ \  __/ |  | |_  *
//*  \__,_|___/___/\___|_|   \__| *
//*                               *
//===----------------------------------------------------------------------===//
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

#ifndef NDEBUG

#    include <array>
#    include <cstdlib>
#    include <iostream>

#    ifdef _WIN32
#        include <tchar.h>
#        define PSTORE_NATIVE_TEXT(str) _TEXT (str)
#    else
#        define PSTORE_NATIVE_TEXT(str) str
#    endif

#    include "pstore/support/gsl.hpp"

#    include "backtrace.hpp"

namespace pstore {

    void assert_failed (gsl::czstring const str, gsl::czstring const file, int const line) {
#    ifdef _WIN32
        auto & out_stream = std::wcerr;
#    else
        auto & out_stream = std::cerr;
#    endif
        out_stream << PSTORE_NATIVE_TEXT ("Assert failed: (") << str
                   << PSTORE_NATIVE_TEXT ("), file ") << file << PSTORE_NATIVE_TEXT (", line ")
                   << line << std::endl;
#    if PSTORE_HAVE_BACKTRACE
        {
            std::array<void *, 64U> callstack;
            void ** const out = callstack.data ();
            int const frames = ::backtrace (out, static_cast<int> (callstack.size ()));

            auto const deleter = [] (void * const p) {
                if (p != nullptr) {
                    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc, hicpp-no-malloc)
                    std::free (p);
                }
            };
            std::unique_ptr<gsl::zstring, decltype (deleter)> const strs{
                ::backtrace_symbols (out, frames), deleter};
            auto * const begin = strs.get ();
            std::copy (
                begin, begin + frames,
                std::ostream_iterator<gsl::czstring> (out_stream, PSTORE_NATIVE_TEXT ("\n")));
        }
#    endif // PSTORE_HAVE_BACKTRACE
        std::abort ();
    }

} // end namespace pstore

#endif // NDEBUG
