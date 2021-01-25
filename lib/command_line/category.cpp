//*            _                               *
//*   ___ __ _| |_ ___  __ _  ___  _ __ _   _  *
//*  / __/ _` | __/ _ \/ _` |/ _ \| '__| | | | *
//* | (_| (_| | ||  __/ (_| | (_) | |  | |_| | *
//*  \___\__,_|\__\___|\__, |\___/|_|   \__, | *
//*                    |___/            |___/  *
//===- lib/command_line/category.cpp --------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/category.hpp"

namespace pstore {
    namespace command_line {

        option_category::option_category (std::string const & title)
                : title_{title} {}

    }     // namespace command_line
} // namespace pstore
