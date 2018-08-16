//*  _                                  _   _              *
//* | |_ _ __ __ _ _ __  ___  __ _  ___| |_(_) ___  _ __   *
//* | __| '__/ _` | '_ \/ __|/ _` |/ __| __| |/ _ \| '_ \  *
//* | |_| | | (_| | | | \__ \ (_| | (__| |_| | (_) | | | | *
//*  \__|_|  \__,_|_| |_|___/\__,_|\___|\__|_|\___/|_| |_| *
//*                                                        *
//===- unittests/mcrepo/transaction.cpp -----------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

#include "transaction.hpp"

using namespace pstore::repo;

auto transaction::allocate (std::size_t size, unsigned /*align*/) -> pstore::address {
    auto ptr = std::shared_ptr<std::uint8_t> (new std::uint8_t[size],
                                              [](std::uint8_t * p) { delete[] p; });
    storage_[ptr.get ()] = ptr;

    static_assert (sizeof (std::uint8_t *) >= sizeof (pstore::address),
                   "expected address to be no larger than a pointer");
    return pstore::address{reinterpret_cast<std::uintptr_t> (ptr.get ())};
}

auto transaction::getrw (pstore::address addr, std::size_t /* size*/) -> std::shared_ptr<void> {
    return storage_.find (reinterpret_cast<std::uint8_t *> (addr.absolute ()))->second;
}

auto transaction::alloc_rw (std::size_t size, unsigned align)
    -> std::pair<std::shared_ptr<void>, pstore::address> {
    auto addr = allocate (size, align);
    auto ptr = getrw (addr, size);
    return std::make_pair (ptr, addr);
}

