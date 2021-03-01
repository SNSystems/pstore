//===- lib/core/base32.cpp ------------------------------------------------===//
//*  _                    _________   *
//* | |__   __ _ ___  ___|___ /___ \  *
//* | '_ \ / _` / __|/ _ \ |_ \ __) | *
//* | |_) | (_| \__ \  __/___) / __/  *
//* |_.__/ \__,_|___/\___|____/_____| *
//*                                   *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file base32.cpp
#include "base32.hpp"

namespace pstore {
    namespace base32 {
        // I've chosen the basic set of 32 characters from RFC4648 to guarantee that the names I've
        // chosen are safe for any operating system.
        std::array<char const, 32> const alphabet{
            {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
             'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '2', '3', '4', '5', '6', '7'}};
    } // namespace base32
} // namespace pstore
