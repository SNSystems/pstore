//===- tools/genromfs/directory_entry.hpp -----------------*- mode: C++ -*-===//
//*      _ _               _                                _               *
//*   __| (_)_ __ ___  ___| |_ ___  _ __ _   _    ___ _ __ | |_ _ __ _   _  *
//*  / _` | | '__/ _ \/ __| __/ _ \| '__| | | |  / _ \ '_ \| __| '__| | | | *
//* | (_| | | | |  __/ (__| || (_) | |  | |_| | |  __/ | | | |_| |  | |_| | *
//*  \__,_|_|_|  \___|\___|\__\___/|_|   \__, |  \___|_| |_|\__|_|   \__, | *
//*                                      |___/                       |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_GENROMFS_DIRECTORY_ENTRY_HPP
#define PSTORE_GENROMFS_DIRECTORY_ENTRY_HPP

#include <ctime>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct directory_entry;
using directory_container = std::vector<directory_entry>;

struct directory_entry {
    directory_entry (std::string name_, unsigned dirno,
                     std::unique_ptr<directory_container> && children_)
            : name (std::move (name_))
            , contents (dirno)
            , modtime{0}
            , children (std::move (children_)) {}
    directory_entry (std::string name_, unsigned fileno, std::time_t modtime_)
            : name (std::move (name_))
            , contents (fileno)
            , modtime{modtime_} {}

    std::string name;
    unsigned contents;
    std::time_t modtime;
    std::unique_ptr<directory_container> children;
};


#endif // PSTORE_GENROMFS_DIRECTORY_ENTRY_HPP
