//*  _   _                 *
//* | |_(_)_ __ ___   ___  *
//* | __| | '_ ` _ \ / _ \ *
//* | |_| | | | | | |  __/ *
//*  \__|_|_| |_| |_|\___| *
//*                        *
//===- include/pstore/core/time.hpp ---------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file time.hpp

#ifndef PSTORE_CORE_TIME_HPP
#define PSTORE_CORE_TIME_HPP

#include <cstdint>

namespace pstore {
    std::uint64_t milliseconds_since_epoch ();
} // namespace pstore

#endif // PSTORE_CORE_TIME_HPP
