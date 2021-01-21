//*                                    _   _                  *
//*  _   _ ___  ___ _ __    ___  _ __ | |_(_) ___  _ __  ___  *
//* | | | / __|/ _ \ '__|  / _ \| '_ \| __| |/ _ \| '_ \/ __| *
//* | |_| \__ \  __/ |    | (_) | |_) | |_| | (_) | | | \__ \ *
//*  \__,_|___/\___|_|     \___/| .__/ \__|_|\___/|_| |_|___/ *
//*                             |_|                           *
//===- include/pstore/vacuum/user_options.hpp -----------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#ifndef PSTORE_VACUUM_OPTIONS_HPP
#define PSTORE_VACUUM_OPTIONS_HPP

#include <string>

namespace vacuum {
    struct user_options {
        bool daemon_mode;
        std::string src_path;
    };
} // namespace vacuum
#endif // PSTORE_VACUUM_OPTIONS_HPP
