//===- tools/write/to_value_pair.cpp --------------------------------------===//
//*  _                     _                          _       *
//* | |_ ___   __   ____ _| |_   _  ___   _ __   __ _(_)_ __  *
//* | __/ _ \  \ \ / / _` | | | | |/ _ \ | '_ \ / _` | | '__| *
//* | || (_) |  \ V / (_| | | |_| |  __/ | |_) | (_| | | |    *
//*  \__\___/    \_/ \__,_|_|\__,_|\___| | .__/ \__,_|_|_|    *
//*                                      |_|                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "to_value_pair.hpp"

#include <cstring>
#include <utility>

#include "pstore/support/assert.hpp"

std::pair<std::string, std::string> to_value_pair (char const * option) {
    std::string key;
    std::string value;

    if (option != nullptr && option[0] != '\0') {
        if (char const * comma = std::strchr (option, ',')) {
            PSTORE_ASSERT (comma >= option);
            key.assign (option, static_cast<std::string::size_type> (comma - option));
            value.assign (comma + 1);
        } else {
            key.assign (option);
        }
    }

    if (key.length () == 0 || value.length () == 0) {
        key.clear ();
        value.clear ();
    }
    return std::make_pair (key, value);
}
