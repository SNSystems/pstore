//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/sieve/switches.hpp -------------------------------------------===//
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
#ifndef SIEVE_SWITCHES_HPP
#define SIEVE_SWITCHES_HPP

#include <memory>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <tchar.h>
#else
typedef char TCHAR;
#endif

#include "config.hpp"

namespace switches {

    struct bad_number : public std::runtime_error {
        bad_number ();
    };
    struct bad_endian : public std::runtime_error {
        bad_endian ();
    };
    struct parse_failure : public std::runtime_error {
        parse_failure ();
    };


    enum class endian {
        native,
        big,
        little,
    };

    struct user_options {
        std::shared_ptr<std::string> output;
        endian endianness;
        unsigned long maximum;
#if defined(_WIN32) && !defined(PSTORE_IS_INSIDE_LLVM)
        static user_options get (int argc, TCHAR * argv[]);
#else
        static user_options get (int argc, char * argv[]);
#endif
    };
}

#endif // SIEVE_SWITCHES_HPP
// eof: tools/sieve/switches.hpp
