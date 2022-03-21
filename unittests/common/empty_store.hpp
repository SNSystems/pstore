//===- unittests/common/empty_store.hpp -------------------*- mode: C++ -*-===//
//*                       _               _                  *
//*   ___ _ __ ___  _ __ | |_ _   _   ___| |_ ___  _ __ ___  *
//*  / _ \ '_ ` _ \| '_ \| __| | | | / __| __/ _ \| '__/ _ \ *
//* |  __/ | | | | | |_) | |_| |_| | \__ \ || (_) | | |  __/ *
//*  \___|_| |_| |_| .__/ \__|\__, | |___/\__\___/|_|  \___| *
//*                |_|        |___/                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef EMPTY_STORE_HPP
#define EMPTY_STORE_HPP

// Standard library includes
#include <cstdint>
#include <cstdlib>
#include <memory>

// 3rd party includes
#include <gmock/gmock.h>

// pstore includes
#include "pstore/core/database.hpp"
#include "pstore/core/transaction.hpp"

class in_memory_store {
public:
    static std::size_t constexpr file_size = pstore::storage::min_region_size * 2;

    // Build an empty, in-memory database.
    in_memory_store ();
    ~in_memory_store ();

    std::shared_ptr<pstore::file::in_memory> const & file () const { return file_; }
    std::shared_ptr<std::uint8_t> const & buffer () const { return buffer_; }

private:
    std::shared_ptr<std::uint8_t> buffer_;
    std::shared_ptr<pstore::file::in_memory> file_;
    static constexpr std::size_t page_size_ = 4096;
};

struct mock_mutex {
    void lock () {}
    void unlock () {}
};

inline pstore::transaction<std::unique_lock<mock_mutex>>
begin (pstore::database & db, std::unique_lock<mock_mutex> && lock) {
    return {db, std::move (lock)};
}

#endif // EMPTY_STORE_HPP
