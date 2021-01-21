//*        _   _ _ _ _          *
//*  _   _| |_(_) (_) |_ _   _  *
//* | | | | __| | | | __| | | | *
//* | |_| | |_| | | | |_| |_| | *
//*  \__,_|\__|_|_|_|\__|\__, | *
//*                      |___/  *
//===- include/pstore/adt/utility.hpp -------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file adt/utility.hpp
/// \brief  Provides definitions needed by the code that are available in C++17 <utility>.

#ifndef PSTORE_ADT_UTILITY_HPP
#define PSTORE_ADT_UTILITY_HPP

namespace pstore {

    struct in_place_t {
        explicit in_place_t () = default;
    };

    constexpr in_place_t in_place{};

} // end namespace pstore

#endif // PSTORE_ADT_UTILITY_HPP
