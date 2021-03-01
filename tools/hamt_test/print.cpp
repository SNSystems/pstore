//===- tools/hamt_test/print.cpp ------------------------------------------===//
//*             _       _    *
//*  _ __  _ __(_)_ __ | |_  *
//* | '_ \| '__| | '_ \| __| *
//* | |_) | |  | | | | | |_  *
//* | .__/|_|  |_|_| |_|\__| *
//* |_|                      *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "print.hpp"
namespace details {
    ios_printer cout{std::cout};
    ios_printer cerr{std::cerr};
} // namespace details
