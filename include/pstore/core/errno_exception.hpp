//*                                                       _   _              *
//*   ___ _ __ _ __ _ __   ___     _____  _____ ___ _ __ | |_(_) ___  _ __   *
//*  / _ \ '__| '__| '_ \ / _ \   / _ \ \/ / __/ _ \ '_ \| __| |/ _ \| '_ \  *
//* |  __/ |  | |  | | | | (_) | |  __/>  < (_|  __/ |_) | |_| | (_) | | | | *
//*  \___|_|  |_|  |_| |_|\___/   \___/_/\_\___\___| .__/ \__|_|\___/|_| |_| *
//*                                                |_|                       *
//===- include/pstore/core/errno_exception.hpp ----------------------------===//
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
/// \file errno_exception.hpp
/// \brief The pstore::errno_exception class provides a wrapper for std::system error which provides
/// additional context for the associated error message.

#ifndef PSTORE_ERRNO_EXCEPTION_HPP
#define PSTORE_ERRNO_EXCEPTION_HPP

#include <exception>
#include <string>
#include <system_error>

namespace pstore {
    /// Provides a wrapper for std::system error which provides additional context for the
    /// associated error message.
    class errno_exception final : public std::system_error {
    public:
        errno_exception (int errcode, char const * message);
        errno_exception (int errcode, std::string const & message);

        errno_exception (errno_exception const &) = default;
        errno_exception (errno_exception &&) = default;
        errno_exception & operator= (errno_exception const &) = default;
        errno_exception & operator= (errno_exception &&) = default;

        ~errno_exception () override;
    };
} // namespace pstore
#endif // PSTORE_ERRNO_EXCEPTION_HPP
