//*  _   _                 *
//* | |_(_)_ __ ___   ___  *
//* | __| | '_ ` _ \ / _ \ *
//* | |_| | | | | | |  __/ *
//*  \__|_|_| |_| |_|\___| *
//*                        *
//===- include/pstore/os/time.hpp -----------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#ifndef PSTORE_OS_TIME_HPP
#define PSTORE_OS_TIME_HPP

#include <ctime>

namespace pstore {

    struct std::tm local_time (std::time_t clock);
    struct std::tm gm_time (std::time_t clock);

} // end namespace pstore

#endif // PSTORE_OS_TIME_HPP
