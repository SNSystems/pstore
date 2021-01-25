//*                                                       _   _              *
//*   ___ _ __ _ __ _ __   ___     _____  _____ ___ _ __ | |_(_) ___  _ __   *
//*  / _ \ '__| '__| '_ \ / _ \   / _ \ \/ / __/ _ \ '_ \| __| |/ _ \| '_ \  *
//* |  __/ |  | |  | | | | (_) | |  __/>  < (_|  __/ |_) | |_| | (_) | | | | *
//*  \___|_|  |_|  |_| |_|\___/   \___/_/\_\___\___| .__/ \__|_|\___/|_| |_| *
//*                                                |_|                       *
//===- include/pstore/core/errno_exception.hpp ----------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file errno_exception.hpp
/// \brief The pstore::errno_exception class provides a wrapper for std::system error which provides
/// additional context for the associated error message.

#ifndef PSTORE_CORE_ERRNO_EXCEPTION_HPP
#define PSTORE_CORE_ERRNO_EXCEPTION_HPP

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
#endif // PSTORE_CORE_ERRNO_EXCEPTION_HPP
