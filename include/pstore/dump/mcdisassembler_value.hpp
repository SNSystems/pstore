//===- include/pstore/dump/mcdisassembler_value.hpp -------*- mode: C++ -*-===//
//*                    _ _                                  _     _            *
//*  _ __ ___   ___ __| (_)___  __ _ ___ ___  ___ _ __ ___ | |__ | | ___ _ __  *
//* | '_ ` _ \ / __/ _` | / __|/ _` / __/ __|/ _ \ '_ ` _ \| '_ \| |/ _ \ '__| *
//* | | | | | | (_| (_| | \__ \ (_| \__ \__ \  __/ | | | | | |_) | |  __/ |    *
//* |_| |_| |_|\___\__,_|_|___/\__,_|___/___/\___|_| |_| |_|_.__/|_|\___|_|    *
//*                                                                            *
//*             _             *
//* __   ____ _| |_   _  ___  *
//* \ \ / / _` | | | | |/ _ \ *
//*  \ V / (_| | | |_| |  __/ *
//*   \_/ \__,_|_|\__,_|\___| *
//*                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_DUMP_MCDISASSEMBLER_VALUE_HPP
#define PSTORE_DUMP_MCDISASSEMBLER_VALUE_HPP

#include <cstdint>

#include "pstore/dump/parameter.hpp"
#include "pstore/dump/value.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace dump {

        value_ptr make_disassembled_value (std::uint8_t const * first, std::uint8_t const * last,
                                           parameters const & parm);

    } // end namespace dump
} // end namespace pstore

#endif // PSTORE_DUMP_MCDISASSEMBLER_VALUE_HPP
