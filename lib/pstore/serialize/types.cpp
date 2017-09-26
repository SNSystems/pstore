//*  _                          *
//* | |_ _   _ _ __   ___  ___  *
//* | __| | | | '_ \ / _ \/ __| *
//* | |_| |_| | |_) |  __/\__ \ *
//*  \__|\__, | .__/ \___||___/ *
//*      |___/|_|               *
//===- lib/pstore/serialize/types.cpp -------------------------------------===//
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
#include "pstore/serialize/types.hpp"
#include <array>
#include <cassert>

namespace pstore {
    namespace serialize {

#ifndef NDEBUG
        void flood (void * ptr, std::size_t size) {
            static std::array<std::uint8_t, 4> const deadbeef {{ 0xDE, 0xAD, 0xBE, 0xEF }};

            auto byte = reinterpret_cast <std::uint8_t *> (ptr);
            auto end = byte + size;

            auto const end1 = byte + (size & ~3U);
            assert (end1 <= end);
            for (; byte < end1; byte += 4) {
                byte [0] = deadbeef [0];
                byte [1] = deadbeef [1];
                byte [2] = deadbeef [2];
                byte [3] = deadbeef [3];
            }

            auto index = 0U;
            for (; byte < end; ++byte) {
                *byte = deadbeef [index];
                ++index;
                if (index >= deadbeef.size ()) {
                    index = 0U;
                }
            }
        }
#endif // NDEBUG

    } // namespace serialize
} // namespace pstore

// eof: lib/pstore/serialize/types.cpp
