//===- include/pstore/command_line/tchar.hpp --------------*- mode: C++ -*-===//
//*  _       _                 *
//* | |_ ___| |__   __ _ _ __  *
//* | __/ __| '_ \ / _` | '__| *
//* | || (__| | | | (_| | |    *
//*  \__\___|_| |_|\__,_|_|    *
//*                            *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_COMMAND_LINE_TCHAR_HPP
#define PSTORE_COMMAND_LINE_TCHAR_HPP

#include <iostream>

#ifdef _WIN32
#    include <tchar.h>
#endif

namespace pstore {
    namespace command_line {

#ifdef _WIN32
#    define NATIVE_TEXT(str) _TEXT (str)
        using tchar = TCHAR;
#else
        using tchar = char;
#    define NATIVE_TEXT(str) str
#endif

#if defined(_WIN32) && defined(_UNICODE)
        extern std::wostream & error_stream;
        extern std::wostream & out_stream;
#else
        extern std::ostream & error_stream;
        extern std::ostream & out_stream;
#endif

    } // end namespace command_line

} // end namespace pstore

#endif // PSTORE_COMMAND_LINE_TCHAR_HPP
