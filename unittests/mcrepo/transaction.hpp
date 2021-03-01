//===- unittests/mcrepo/transaction.hpp -------------------*- mode: C++ -*-===//
//*  _                                  _   _              *
//* | |_ _ __ __ _ _ __  ___  __ _  ___| |_(_) ___  _ __   *
//* | __| '__/ _` | '_ \/ __|/ _` |/ __| __| |/ _ \| '_ \  *
//* | |_| | | (_| | | | \__ \ (_| | (__| |_| | (_) | | | | *
//*  \__|_|  \__,_|_| |_|___/\__,_|\___|\__|_|\___/|_| |_| *
//*                                                        *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef MCREPO_TRANSACTION_HPP
#define MCREPO_TRANSACTION_HPP

#include "gmock/gmock.h"
#include <memory>
#include <vector>

#include "pstore/core/address.hpp"

class transaction {
public:
    using storage_type = std::map<std::uint8_t *, std::shared_ptr<std::uint8_t>>;

    auto allocate (std::size_t size, unsigned align) -> pstore::address;

    auto getrw (pstore::address addr, std::size_t size) -> std::shared_ptr<void>;

    auto alloc_rw (std::size_t size, unsigned align)
        -> std::pair<std::shared_ptr<void>, pstore::address>;

    storage_type const & get_storage () const { return storage_; }

private:
    storage_type storage_;
};

#endif // MCREPO_TRANSACTION_HPP
