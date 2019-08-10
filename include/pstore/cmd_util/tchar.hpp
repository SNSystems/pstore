//*  _       _                 *
//* | |_ ___| |__   __ _ _ __  *
//* | __/ __| '_ \ / _` | '__| *
//* | || (__| | | | (_| | |    *
//*  \__\___|_| |_|\__,_|_|    *
//*                            *
//===- include/pstore/cmd_util/tchar.hpp ----------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_CMD_UTIL_TCHAR_HPP
#define PSTORE_CMD_UTIL_TCHAR_HPP

#include <iostream>

#ifdef _WIN32
#    include <tchar.h>
#endif

namespace pstore {
    namespace cmd_util {

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

    } // end namespace cmd_util

} // end namespace pstore

#endif // PSTORE_CMD_UTIL_TCHAR_HPP
