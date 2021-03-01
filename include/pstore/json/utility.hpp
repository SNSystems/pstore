//===- include/pstore/json/utility.hpp --------------------*- mode: C++ -*-===//
//*        _   _ _ _ _          *
//*  _   _| |_(_) (_) |_ _   _  *
//* | | | | __| | | | __| | | | *
//* | |_| | |_| | | | |_| |_| | *
//*  \__,_|\__|_|_|_|\__|\__, | *
//*                      |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_JSON_UTILITY_HPP
#define PSTORE_JSON_UTILITY_HPP

#include <string>

namespace pstore {
    namespace json {

        bool is_valid (std::string const & str);

    } // end namespace json
} // end namespace pstore

#endif // PSTORE_JSON_UTILITY_HPP
