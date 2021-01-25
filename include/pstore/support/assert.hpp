//*                          _    *
//*   __ _ ___ ___  ___ _ __| |_  *
//*  / _` / __/ __|/ _ \ '__| __| *
//* | (_| \__ \__ \  __/ |  | |_  *
//*  \__,_|___/___/\___|_|   \__| *
//*                               *
//===- include/pstore/support/assert.hpp ----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file assert.hpp
/// \brief An implementation of the standard assert() macro with the exception that it will, on
/// failure, dump a backtrace on platforms with the appropriate support library.

#ifndef PSTORE_SUPPORT_ASSERT_HPP
#define PSTORE_SUPPORT_ASSERT_HPP

#include "pstore/support/portab.hpp"

namespace pstore {

    PSTORE_NO_RETURN
    void assert_failed (char const * str, char const * file, int line);

} // end namespace pstore

#ifndef NDEBUG
#    define PSTORE_ASSERT(expr)                                                                    \
        ((expr) ? static_cast<void> (0) : ::pstore::assert_failed (#expr, __FILE__, __LINE__))
#else
#    define PSTORE_ASSERT(x)
#endif // NDEBUG

#endif // PSTORE_SUPPORT_ASSERT_HPP
