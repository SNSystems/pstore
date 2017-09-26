//*  _     _                         *
//* | |__ (_)_ __   __ _ _ __ _   _  *
//* | '_ \| | '_ \ / _` | '__| | | | *
//* | |_) | | | | | (_| | |  | |_| | *
//* |_.__/|_|_| |_|\__,_|_|   \__, | *
//*                           |___/  *
//===- unittests/pstore/binary.hpp ----------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc. 
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
#ifndef BINARY_HPP
#define BINARY_HPP

#include <cstdint>
#include <cstdlib>


/// The binary template is used to write binary constants (or at least it is until we can fully
/// switch to C++14). The first template argument is the type of the constant; each of the remaining
/// arguments must be either 0 or 1 to represent the value at each bit position. The most significant
/// bit is written first.
template <typename T, unsigned V0, unsigned... Values>
struct binary {
    static_assert (V0 == 0 || V0 == 1, "bit value must be 0 or 1");
    static constexpr T value = (T{V0} << sizeof...(Values)) | binary<T, Values...>::value;
    static constexpr std::size_t length = sizeof...(Values) + 1;
};
template <typename T, unsigned V0, unsigned... Values>
constexpr T binary<T, V0, Values...>::value;

template <typename T, unsigned V0, unsigned... Values>
constexpr std::size_t binary<T, V0, Values...>::length;


template <typename T>
struct binary<T, 0> {
    static constexpr T value = 0;
    static constexpr std::size_t length = 1;
};
template <typename T>
constexpr T binary<T, 0>::value;
template <typename T>
constexpr std::size_t binary<T, 0>::length;


template <typename T>
struct binary<T, 1> {
    static constexpr T value = 1;
    static constexpr std::size_t length = 1;
};
template <typename T>
constexpr T binary<T, 1>::value;
template <typename T>
constexpr std::size_t binary<T, 1>::length;


static_assert (binary<unsigned, 0, 0>::value == 0U, "0b0 should be 0");
static_assert (binary<unsigned, 0, 0>::length == 2, "0b00 length should be 2");

static_assert (binary<unsigned, 0, 1>::value == 1U, "0b1 should be 1");
static_assert (binary<unsigned, 0, 0, 0, 0, 0, 0, 0, 1>::value == 1U, "0b00000001 should be 1");
static_assert (binary<unsigned, 1, 0>::value == 2, "0b10 should be 2");
static_assert (binary<unsigned, 1, 1>::value == 3U, "0b11 should be 3");
static_assert (binary<unsigned, 1, 0, 0>::value == 4U, "0b100 should be 4");

static_assert (binary<unsigned, 1, 0, 0, 0, 0, 0, 0, 0>::value == 128U, "0b10000000 should be 128");
static_assert (binary<unsigned, 1, 0, 0, 0, 0, 0, 0, 0>::length == 8,
               "0b10000000 length should be 8");

#endif // BINARY_HPP
// eof: unittests/pstore/binary.hpp
