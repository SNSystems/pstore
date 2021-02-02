//*                       _               _                  *
//*   ___ _ __ ___  _ __ | |_ _   _   ___| |_ ___  _ __ ___  *
//*  / _ \ '_ ` _ \| '_ \| __| | | | / __| __/ _ \| '__/ _ \ *
//* |  __/ | | | | | |_) | |_| |_| | \__ \ || (_) | | |  __/ *
//*  \___|_| |_| |_| .__/ \__|\__, | |___/\__\___/|_|  \___| *
//*                |_|        |___/                          *
//===- unittests/common/empty_store.cpp -----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "empty_store.hpp"
#include <cstdlib>
#include "pstore/support/error.hpp"

constexpr std::size_t InMemoryStore::file_size;
constexpr std::size_t InMemoryStore::page_size_;

InMemoryStore::InMemoryStore ()
        : buffer_{pstore::aligned_valloc (file_size, page_size_)}
        , file_{std::make_shared<pstore::file::in_memory> (buffer_, file_size)} {
    pstore::database::build_new_store (*file_);
}

InMemoryStore::~InMemoryStore () = default;


EmptyStoreFile::EmptyStoreFile ()
        : file_{std::make_shared<pstore::file::file_handle> ()} {}

EmptyStoreFile::~EmptyStoreFile () = default;
