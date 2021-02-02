//*                                             *
//*   ___ ___  _ __ ___  _ __ ___   ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
//* | (_| (_) | | | | | | | | | | | (_) | | | | *
//*  \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
//*                                             *
//===- unittests/support/klee/uint128/common.hpp --------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_UNITTEST_SUPPORT_KLEE_UINT128
#define PSTORE_UNITTEST_SUPPORT_KLEE_UINT128

#include <cinttypes>
#include <cstdint>
#include <cstdio>

constexpr inline __uint128_t to_native (pstore::uint128 v) noexcept {
    return __uint128_t{v.high ()} << 64 | __uint128_t{v.low ()};
}

constexpr inline std::uint64_t max64 () noexcept {
    return static_cast<std::uint64_t> ((__uint128_t{1} << 64) - 1U);
}

constexpr inline std::uint64_t high (__uint128_t v) noexcept {
    return static_cast<std::uint64_t> ((v >> 64) & max64 ());
}

constexpr inline std::uint64_t high (pstore::uint128 v) noexcept {
    return v.high ();
}

constexpr inline std::uint64_t low (__uint128_t v) noexcept {
    return static_cast<std::uint64_t> (v & max64 ());
}

constexpr inline std::uint64_t low (pstore::uint128 v) noexcept {
    return v.low ();
}

template <typename T>
void dump_uint128 (T v) {
    std::printf ("0x%" PRIx64 ",0x%" PRIx64, high (v), low (v));
}

template <typename T>
void dump_uint128 (char const * n1, T v1) {
    std::fputs (n1, stdout);
    dump_uint128 (v1);
}

template <typename T>
void dump_uint128 (char const * n1, T v1, char const * n2, T v2) {
    dump_uint128 (n1, v1);
    std::fputc (' ', stdout);
    dump_uint128 (n2, v2);
    std::fputc ('\n', stdout);
}

#endif // PSTORE_UNITTEST_SUPPORT_KLEE_UINT128
