//===- tools/lock_test/say.hpp ----------------------------*- mode: C++ -*-===//
//*                   *
//*  ___  __ _ _   _  *
//* / __|/ _` | | | | *
//* \__ \ (_| | |_| | *
//* |___/\__,_|\__, | *
//*            |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file say.hpp
/// \brief A wrapper around ostream operator<< which takes a lock to prevent output from different
/// threads from being interleaved.

#ifndef PSTORE_LOCK_TEST_SAY_HPP
#define PSTORE_LOCK_TEST_SAY_HPP

#include <mutex>
#include <ostream>

template <typename OStream, typename Arg>
inline void say_impl (OStream & os, Arg a) {
    os << a;
}
template <typename OStream, typename Arg, typename... Args>
inline void say_impl (OStream & os, Arg a, Args... args) {
    os << a;
    say_impl (os, args...);
}

template <typename OStream, typename... Args>
void say (OStream & os, Args... args) {
    static std::mutex io_mut;
    std::lock_guard<std::mutex> lock{io_mut};
    say_impl (os, args...);
    os << std::endl;
}

#endif // PSTORE_LOCK_TEST_SAY_HPP
