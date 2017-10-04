//*                   _        _      *
//*  _ __   ___  _ __| |_ __ _| |__   *
//* | '_ \ / _ \| '__| __/ _` | '_ \  *
//* | |_) | (_) | |  | || (_| | |_) | *
//* | .__/ \___/|_|   \__\__,_|_.__/  *
//* |_|                               *
//===- include/pstore_support/portab.hpp ----------------------------------===//
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
#ifndef PSTORE_SUPPORT_PORTAB_HPP
#define PSTORE_SUPPORT_PORTAB_HPP

#ifdef __cpp_exceptions
#define PSTORE_CPP_EXCEPTIONS  __cpp_exceptions
#elif defined (_MSC_VER) && defined (_CPPUNWIND)
#define PSTORE_CPP_EXCEPTIONS 199711
#else
#undef PSTORE_CPP_EXCEPTIONS
#endif


#ifdef __cpp_rtti
#define PSTORE_CPP_RTTI  __cpp_rtti
#elif defined (_MSC_VER) && defined (_CPPRTTI)
#define PSTORE_CPP_RTTI 199711
#else
#undef PSTORE_CPP_RTTI
#endif


#ifdef __has_cpp_attribute
#define PSTORE_HAS_STANDARD_NORETURN_ATTRIBUTE  __has_cpp_attribute (noreturn)
#else
#define PSTORE_HAS_STANDARD_NORETURN_ATTRIBUTE  0
#endif

#if PSTORE_HAS_STANDARD_NORETURN_ATTRIBUTE
    #define PSTORE_NO_RETURN  [[noreturn]]
#elif defined (_MSC_VER)
    #define PSTORE_NO_RETURN  __declspec(noreturn)
#else
    #define PSTORE_NO_RETURN  __attribute__(noreturn)
#endif

#ifdef _WIN32
    #ifdef min
        #undef min
    #endif
    #ifdef max
        #undef max
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif

#endif // PSTORE_SUPPORT_PORTAB_HPP
//eof:include/pstore_support/portab.hpp

// eof: include/pstore_support/portab.hpp
