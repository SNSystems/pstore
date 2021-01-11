//*      _     _  __ _   _       __ _    *
//*  ___| |__ (_)/ _| |_| | ___ / _| |_  *
//* / __| '_ \| | |_| __| |/ _ \ |_| __| *
//* \__ \ | | | |  _| |_| |  __/  _| |_  *
//* |___/_| |_|_|_|  \__|_|\___|_|  \__| *
//*                                      *
//===- unittests/support/klee/uint128/shiftleft.cpp -----------------------===//
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
#include <cassert>
#include <klee/klee.h>
#include "pstore/support/uint128.hpp"
#include "./common.hpp"

int main (int argc, char * argv[]) {
    pstore::uint128 value;
    unsigned dist;
    klee_make_symbolic (&value, sizeof (value), "value");
    klee_make_symbolic (&dist, sizeof (dist), "dist");
    klee_assume (dist < 128);

    pstore::uint128 const result = value << dist;
    __uint128_t const resultx = to_native (value) << dist;

#if 1 // PSTORE_KLEE_RUN
    std::fputs ("value:", stdout);
    dump_uint128 ("value:", value);
    std::printf (" dist:0x%x\n", dist);
#endif

    assert (result == resultx);
}
