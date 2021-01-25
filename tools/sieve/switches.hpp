//*               _ _       _                *
//*  _____      _(_) |_ ___| |__   ___  ___  *
//* / __\ \ /\ / / | __/ __| '_ \ / _ \/ __| *
//* \__ \\ V  V /| | || (__| | | |  __/\__ \ *
//* |___/ \_/\_/ |_|\__\___|_| |_|\___||___/ *
//*                                          *
//===- tools/sieve/switches.hpp -------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef SIEVE_SWITCHES_HPP
#define SIEVE_SWITCHES_HPP

#include <memory>
#include <stdexcept>
#include <string>

#include "pstore/command_line/tchar.hpp"

enum class endian {
    native,
    big,
    little,
};

struct user_options {
    std::string output;
    endian endianness;
    unsigned long maximum;

    static user_options get (int argc, pstore::command_line::tchar * argv[]);
};

#endif // SIEVE_SWITCHES_HPP
