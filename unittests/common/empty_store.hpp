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

#include <cstdint>
#include <cstdlib>
#include <memory>

#include <gmock/gmock.h>

#include "pstore/core/database.hpp"
#include "pstore/core/transaction.hpp"

class InMemoryStore {
public:
    static std::size_t constexpr file_size = pstore::storage::min_region_size * 2;

    // Build an empty, in-memory database.
    InMemoryStore ();
    ~InMemoryStore ();

    std::shared_ptr<pstore::file::in_memory> const & file () const { return file_; }
    std::shared_ptr<std::uint8_t> const & buffer () const { return buffer_; }

private:
    std::shared_ptr<std::uint8_t> buffer_;
    std::shared_ptr<pstore::file::in_memory> file_;
    static constexpr std::size_t page_size_ = 4096;
};

// TODO: remove this class and prefer aggregation of InMemoryStore.
class EmptyStore : public ::testing::Test, public InMemoryStore {
};

class EmptyStoreFile : public ::testing::Test {
public:
    // Build an empty, file database.
    EmptyStoreFile ();
    ~EmptyStoreFile () override;

protected:
    std::shared_ptr<pstore::file::file_handle> const & file () { return file_; }

private:
    std::shared_ptr<pstore::file::file_handle> file_;
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
