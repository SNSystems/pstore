//*                                _    *
//*   ___ ___  _ ____   _____ _ __| |_  *
//*  / __/ _ \| '_ \ \ / / _ \ '__| __| *
//* | (_| (_) | | | \ V /  __/ |  | |_  *
//*  \___\___/|_| |_|\_/ \___|_|   \__| *
//*                                     *
//===- unittests/dump/convert.hpp -----------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#ifndef CONVERT_HPP
#define CONVERT_HPP

#include <cstdlib>
#include <cstring>
#include <string>

template <typename CharType>
std::basic_string<CharType> convert (char const * str);
template <>
std::basic_string<char> convert (char const * str);
template <>
std::basic_string<wchar_t> convert (char const * str);

#endif // CONVERT_HPP
