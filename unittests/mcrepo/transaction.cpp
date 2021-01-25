//*  _                                  _   _              *
//* | |_ _ __ __ _ _ __  ___  __ _  ___| |_(_) ___  _ __   *
//* | __| '__/ _` | '_ \/ __|/ _` |/ __| __| |/ _ \| '_ \  *
//* | |_| | | (_| | | | \__ \ (_| | (__| |_| | (_) | | | | *
//*  \__|_|  \__,_|_| |_|___/\__,_|\___|\__|_|\___/|_| |_| *
//*                                                        *
//===- unittests/mcrepo/transaction.cpp -----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "transaction.hpp"

auto transaction::allocate (std::size_t size, unsigned /*align*/) -> pstore::address {
    auto ptr = std::shared_ptr<std::uint8_t> (new std::uint8_t[size],
                                              [] (std::uint8_t * p) { delete[] p; });
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
