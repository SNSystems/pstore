//*                    _      _                 _ _             *
//*  _ __ ___   ___ __| | ___| |__  _   _  __ _| (_)_ __   ___  *
//* | '_ ` _ \ / __/ _` |/ _ \ '_ \| | | |/ _` | | | '_ \ / _ \ *
//* | | | | | | (_| (_| |  __/ |_) | |_| | (_| | | | | | |  __/ *
//* |_| |_| |_|\___\__,_|\___|_.__/ \__,_|\__, |_|_|_| |_|\___| *
//*                                       |___/                 *
//*             _             *
//* __   ____ _| |_   _  ___  *
//* \ \ / / _` | | | | |/ _ \ *
//*  \ V / (_| | | |_| |  __/ *
//*   \_/ \__,_|_|\__,_|\___| *
//*                           *
//===- lib/dump/mcdebugline_value.cpp -------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/dump/mcdebugline_value.hpp"

#include "pstore/config/config.hpp"

namespace pstore {
    namespace dump {

        value_ptr make_debuglineheader_value (std::uint8_t const * first, std::uint8_t const * last,
                                              bool const hex_mode) {
            if (hex_mode) {
                return std::make_shared<binary16> (first, last);
            }
            return std::make_shared<binary> (first, last);
        }

    } // namespace dump
} // namespace pstore
