//===- tools/write/to_value_pair.hpp ----------------------*- mode: C++ -*-===//
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
/// \brief to_value_pair.hpp
#ifndef TO_VALUE_PAIR_HPP
#define TO_VALUE_PAIR_HPP

#include <string>

/// Splits a null-terminated character string into two pieces at the first
/// instance of a comma. If no comma is found, or the first character is
/// a comma, then this is an error and two zero-length strings are returned.
std::pair<std::string, std::string> to_value_pair (char const * option);

inline std::pair<std::string, std::string> to_value_pair (std::string const & option) {
    return to_value_pair (option.c_str ());
}

#endif // TO_VALUE_PAIR_HPP
