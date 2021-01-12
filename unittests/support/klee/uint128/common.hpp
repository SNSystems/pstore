//*                                             *
//*   ___ ___  _ __ ___  _ __ ___   ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
//* | (_| (_) | | | | | | | | | | | (_) | | | | *
//*  \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
//*                                             *
//===- unittests/support/klee/uint128/common.hpp --------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
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
