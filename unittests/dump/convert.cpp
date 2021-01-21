//*                                _    *
//*   ___ ___  _ ____   _____ _ __| |_  *
//*  / __/ _ \| '_ \ \ / / _ \ '__| __| *
//* | (_| (_) | | | \ V /  __/ |  | |_  *
//*  \___\___/|_| |_|\_/ \___|_|   \__| *
//*                                     *
//===- unittests/dump/convert.cpp -----------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#include "convert.hpp"

template <>
std::basic_string<char> convert (char const * str) {
    return {str};
}
template <>
std::basic_string<wchar_t> convert (char const * str) {
    std::basic_string<wchar_t> result (std::strlen (str) + 1, L'\0');
    std::size_t bytes_written = 0;
    for (;;) {
        std::size_t len = result.length ();
        bytes_written = std::mbstowcs (&result[0], str, len);
        if (bytes_written < len) {
            result.resize (bytes_written);
            break;
        }
        len += len / 2U;
        result.resize (len, L'\0');
    }
    return result;
}
