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
//===- include/pstore/dump/mcdebugline_value.hpp --------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_DUMP_MCDEBUGLINE_VALUE_HPP
#define PSTORE_DUMP_MCDEBUGLINE_VALUE_HPP

#include <cstdint>
#include "pstore/dump/value.hpp"

namespace pstore {
    namespace dump {

        value_ptr make_debuglineheader_value (std::uint8_t const * first, std::uint8_t const * last,
                                              bool hex_mode);

    } // namespace dump
} // namespace pstore

#endif // PSTORE_DUMP_MCDEBUGLINE_VALUE_HPP
