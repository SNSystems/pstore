//===- lib/core/errno_exception.cpp ---------------------------------------===//
//*                                                       _   _              *
//*   ___ _ __ _ __ _ __   ___     _____  _____ ___ _ __ | |_(_) ___  _ __   *
//*  / _ \ '__| '__| '_ \ / _ \   / _ \ \/ / __/ _ \ '_ \| __| |/ _ \| '_ \  *
//* |  __/ |  | |  | | | | (_) | |  __/>  < (_|  __/ |_) | |_| | (_) | | | | *
//*  \___|_|  |_|  |_| |_|\___/   \___/_/\_\___\___| .__/ \__|_|\___/|_| |_| *
//*                                                |_|                       *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/core/errno_exception.hpp"

namespace pstore {
    errno_exception::errno_exception (int const errcode, char const * const message)
            : std::system_error (std::error_code (errcode, std::generic_category ()), message) {}

    errno_exception::errno_exception (int const errcode, std::string const & message)
            : std::system_error (std::error_code (errcode, std::generic_category ()), message) {}

    errno_exception::~errno_exception () = default;
} // namespace pstore
