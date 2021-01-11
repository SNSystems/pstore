//*                    _           _  *
//*   __ _ _   _  ___ | |_ ___  __| | *
//*  / _` | | | |/ _ \| __/ _ \/ _` | *
//* | (_| | |_| | (_) | ||  __/ (_| | *
//*  \__, |\__,_|\___/ \__\___|\__,_| *
//*     |_|                           *
//===- include/pstore/support/quoted.hpp ----------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
/// \file quoted.hpp
/// \brief Wrapping quotation marks around strings for output to the user.

#ifndef PSTORE_SUPPORT_QUOTED_HPP
#define PSTORE_SUPPORT_QUOTED_HPP

#include <iomanip>

#include "pstore/support/gsl.hpp"

namespace pstore {

    /// Wraps quotation marks around a string for presentation to the user. Intended for use
    /// as an io-manipulator.
    ///
    /// \param str The string to be wrapped in quotation marks.
    inline auto quoted (gsl::czstring const str) { return std::quoted (str, '"', '\0'); }

    /// Wraps quotation marks around a string for presentation to the user. Intended for use
    /// as an io-manipulator.
    ///
    /// \param str The string to be wrapped in quotation marks.
    inline auto quoted (std::string const & str) { return std::quoted (str, '"', '\0'); }

} // end namespace pstore

#endif // PSTORE_SUPPORT_QUOTED_HPP
