//===- tools/genromfs/dump_tree.hpp -----------------------*- mode: C++ -*-===//
//*      _                         _                  *
//*   __| |_   _ _ __ ___  _ __   | |_ _ __ ___  ___  *
//*  / _` | | | | '_ ` _ \| '_ \  | __| '__/ _ \/ _ \ *
//* | (_| | |_| | | | | | | |_) | | |_| | |  __/  __/ *
//*  \__,_|\__,_|_| |_| |_| .__/   \__|_|  \___|\___| *
//*                       |_|                         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_GENROMFS_DUMP_TREE_HPP
#define PSTORE_GENROMFS_DUMP_TREE_HPP

#include <iosfwd>
#include <unordered_set>

#include "./directory_entry.hpp"

void dump_tree (std::ostream & os, std::unordered_set<unsigned> & forwards,
                directory_container const & dir, unsigned id, unsigned parent_id);

#endif // PSTORE_GENROMFS_DUMP_TREE_HPP
