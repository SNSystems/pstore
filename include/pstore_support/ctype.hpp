//*       _                     *
//*   ___| |_ _   _ _ __   ___  *
//*  / __| __| | | | '_ \ / _ \ *
//* | (__| |_| |_| | |_) |  __/ *
//*  \___|\__|\__, | .__/ \___| *
//*           |___/|_|          *
//===- include/pstore_support/ctype.hpp -----------------------------------===//
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
/// \file ctype.hpp

#ifndef PSTORE_SUPPORT_CTYPE_HPP
#define PSTORE_SUPPORT_CTYPE_HPP

#include <cctype>
#include <cwctype>

namespace pstore {

    inline bool isspace (char c) {
        // std::isspace() has undefined behavior if the input value is not representable as unsigned
        // char and not not equal to EOF.
        auto const uc = static_cast<unsigned char> (c);
        // TODO: std::isspace()'s behavior is affected by the current locale, but our "locale" is
        // always UTF-8.
        return std::isspace (static_cast<int> (uc)) != 0;
    }

    inline bool isspace (wchar_t c) { return std::iswspace (c) != 0; }

} // namespace pstore

#endif // PSTORE_SUPPORT_CTYPE_HPP
// eof: include/pstore_support/ctype.hpp
