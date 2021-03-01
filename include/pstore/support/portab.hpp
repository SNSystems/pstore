//===- include/pstore/support/portab.hpp ------------------*- mode: C++ -*-===//
//*                   _        _      *
//*  _ __   ___  _ __| |_ __ _| |__   *
//* | '_ \ / _ \| '__| __/ _` | '_ \  *
//* | |_) | (_) | |  | || (_| | |_) | *
//* | .__/ \___/|_|   \__\__,_|_.__/  *
//* |_|                               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef PSTORE_SUPPORT_PORTAB_HPP
#define PSTORE_SUPPORT_PORTAB_HPP

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#endif // _WIN32

#include "pstore/config/config.hpp"

#ifdef PSTORE_EXCEPTIONS
#    define PSTORE_TRY try
#    define PSTORE_CATCH(x, code) catch (x) code
#else
#    define PSTORE_TRY
#    define PSTORE_CATCH(x, code)
#endif

namespace pstore {

    template <typename Function>
    void no_ex_escape (Function fn) {
        // Just call fn and catch any exception it may throw.
        // clang-format off
        PSTORE_TRY {
            fn ();
        }
        PSTORE_CATCH (..., {
            //! OCLINT(PH - don't warn about an empty catch)
        })
        // clang-format on
    }

} // end namespace pstore

/// PSTORE_CPP_RTTI: Defined if RTTI is enabled.
#ifdef __cpp_rtti
#    define PSTORE_CPP_RTTI __cpp_rtti
#elif defined(_MSC_VER) && defined(_CPPRTTI)
#    define PSTORE_CPP_RTTI 199711
#else
#    undef PSTORE_CPP_RTTI
#endif

#ifndef __has_builtin
#    define __has_builtin(x) 0
#endif
#ifndef __has_cpp_attribute
#    define __has_cpp_attribute(x) 0
#endif

/// A function-like feature checking macro that is a wrapper around
/// `__has_attribute`, which is defined by GCC 5+ and Clang and evaluates to a
/// nonzero constant integer if the attribute is supported or 0 if not.
/// It evaluates to zero if `__has_attribute` is not defined by the compiler.
#ifdef __has_attribute
#    define PSTORE_HAS_ATTRIBUTE(x) __has_attribute (x)
#else
#    define PSTORE_HAS_ATTRIBUTE(x) false
#endif

/// A macro which is a wrapper around __builtin_expect() which is available with both GCC and Clang
/// to tell the compiler whether a particular branch is likely to be taken.
#if __has_builtin(__builtin_expect)
#    define PSTORE_LIKELY(expr) __builtin_expect ((expr), true)
#    define PSTORE_UNLIKELY(expr) __builtin_expect ((expr), false)
#else
#    define PSTORE_LIKELY(expr) (expr)
#    define PSTORE_UNLIKELY(expr) (expr)
#endif


// Specifies that the function does not return.
#if __has_cpp_attribute(noreturn)
#    define PSTORE_NO_RETURN [[noreturn]]
#elif PSTORE_HAVE_ATTRIBUTE_NORETURN
#    define PSTORE_NO_RETURN __attribute__ ((noreturn))
#elif defined(_MSC_VER)
#    define PSTORE_NO_RETURN __declspec(noreturn)
#else
#    define PSTORE_NO_RETURN
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

#ifndef __has_feature          // Optional of course.
#    define __has_feature(x) 0 // Compatibility with non-clang compilers.
#endif
#ifndef __has_extension
#    define __has_extension __has_feature // Compatibility with pre-3.0 compilers.
#endif

#ifndef __has_warning
#    define __has_warning(x) 0 // Compatibility with non-clang compilers.
#endif

/// PSTORE_FALLTHROUGH - Mark fallthrough cases in switch statements.
#if __cplusplus > 201402L && __has_cpp_attribute(fallthrough)
#    define PSTORE_FALLTHROUGH [[fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#    define PSTORE_FALLTHROUGH [[gnu::fallthrough]]
#elif defined(_MSC_VER)
// MSVC's __fallthrough annotations are checked by /analyze (Code Analysis):
// https://msdn.microsoft.com/en-us/library/ms235402%28VS.80%29.aspx
#    include <sal.h>
#    define PSTORE_FALLTHROUGH __fallthrough
#elif !__cplusplus
// Workaround for llvm.org/PR23435, since clang 3.6 and below emit a spurious
// error when __has_cpp_attribute is given a scoped attribute in C mode.
#    define PSTORE_FALLTHROUGH
#elif __clang__ && __has_feature(cxx_attributes) && __has_warning("-Wimplicit-fallthrough")
#    define PSTORE_FALLTHROUGH [[clang::fallthrough]]
#else
#    define PSTORE_FALLTHROUGH
#endif


// Use of the _Nonnull/_Nullable keywords draws an warning from clang ("Type nullability specifier
// is a Clang extention") which the LLVM build does not suppress.
#if defined(PSTORE_HAVE_NONNULL_KEYWORD) && !defined(PSTORE_IS_INSIDE_LLVM)
#    define PSTORE_NONNULL _Nonnull
#else
#    define PSTORE_NONNULL
#endif
#if defined(PSTORE_HAVE_NULLABLE_KEYWORD) && !defined(PSTORE_IS_INSIDE_LLVM)
#    define PSTORE_NULLABLE _Nullable
#else
#    define PSTORE_NULLABLE
#endif

#ifdef _WIN32
using ssize_t = SSIZE_T;
#endif

#endif // PSTORE_SUPPORT_PORTAB_HPP
