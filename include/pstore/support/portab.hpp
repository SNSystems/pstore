//*                   _        _      *
//*  _ __   ___  _ __| |_ __ _| |__   *
//* | '_ \ / _ \| '__| __/ _` | '_ \  *
//* | |_) | (_) | |  | || (_| | |_) | *
//* | .__/ \___/|_|   \__\__,_|_.__/  *
//* |_|                               *
//===- include/pstore/support/portab.hpp ----------------------------------===//
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

#ifndef PSTORE_SUPPORT_PORTAB_HPP
#define PSTORE_SUPPORT_PORTAB_HPP

#ifdef __cpp_exceptions
#    define PSTORE_CPP_EXCEPTIONS __cpp_exceptions
#elif defined(_MSC_VER) && defined(_CPPUNWIND)
#    define PSTORE_CPP_EXCEPTIONS 199711
#else
#    undef PSTORE_CPP_EXCEPTIONS
#endif

#ifdef PSTORE_CPP_EXCEPTIONS
#    define PSTORE_TRY try
#    define PSTORE_CATCH(x, code) catch (x) code
#else
#    define PSTORE_TRY
#    define PSTORE_CATCH(x, code)
#endif

#define PSTORE_NO_EX_ESCAPE(x)                                                                     \
    do {                                                                                           \
        PSTORE_TRY { x; }                                                                          \
        PSTORE_CATCH (..., {})                                                                     \
    } while (0)


/// PSTORE_CPP_RTTI: Defined if RTTI is enabled.
#ifdef __cpp_rtti
#    define PSTORE_CPP_RTTI __cpp_rtti
#elif defined(_MSC_VER) && defined(_CPPRTTI)
#    define PSTORE_CPP_RTTI 199711
#else
#    undef PSTORE_CPP_RTTI
#endif

#ifndef __has_cpp_attribute
#    define __has_cpp_attribute(x) 0
#endif

#if __has_cpp_attribute(noreturn)
#    define PSTORE_NO_RETURN [[noreturn]]
#elif defined(_MSC_VER)
#    define PSTORE_NO_RETURN __declspec(noreturn)
#else
#    define PSTORE_NO_RETURN __attribute__ (noreturn)
#endif

#ifdef _WIN32
#    ifdef min
#        undef min
#    endif
#    ifdef max
#        undef max
#    endif
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#endif

#ifndef PSTORE_STATIC_ASSERT
    /// A single-argument version of static_assert. Remove once we can run the compiler in C++14
    /// mode.
#    define PSTORE_STATIC_ASSERT(x) static_assert (x, #    x)
#endif

/// PSTORE_FALLTHROUGH - Mark fallthrough cases in switch statements.
#if __cplusplus > 201402L && __has_cpp_attribute(fallthrough)
#    define PSTORE_FALLTHROUGH [[fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#    define PSTORE_FALLTHROUGH [[gnu::fallthrough]]
#elif !__cplusplus
    // Workaround for llvm.org/PR23435, since clang 3.6 and below emit a spurious
    // error when __has_cpp_attribute is given a scoped attribute in C mode.
#    define PSTORE_FALLTHROUGH
#elif __has_cpp_attribute(clang::fallthrough)
#    define PSTORE_FALLTHROUGH [[clang::fallthrough]]
#elif defined(_MSC_VER)
    // MSVC's __fallthrough annotations are checked by /analyze (Code Analysis):
    // https://msdn.microsoft.com/en-us/library/ms235402%28VS.80%29.aspx
#    include <sal.h>
#    define PSTORE_FALLTHROUGH __fallthrough
#else
#    define PSTORE_FALLTHROUGH
#endif
#endif // PSTORE_SUPPORT_PORTAB_HPP
