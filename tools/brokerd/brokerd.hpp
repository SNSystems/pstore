//===- tools/brokerd/brokerd.hpp ---------------------------------------------===//
//*  _               _                _  *
//* | |             | |              | | *
//* | |__  _ __ ___ | | _____ _ __ __| | *
//* | '_ \| '__/ _ \| |/ / _ \ '__/ _` | *
//* | |_) | | | (_) |   <  __/ | | (_| | *
//* |_.__/|_|  \___/|_|\_\___|_|  \__,_| *
//*                                      *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

namespace pstore {
    namespace broker {
        int run_broker (switches const & opt);
    }
} // namespace pstore
