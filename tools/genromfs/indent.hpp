//*  _           _            _    *
//* (_)_ __   __| | ___ _ __ | |_  *
//* | | '_ \ / _` |/ _ \ '_ \| __| *
//* | | | | | (_| |  __/ | | | |_  *
//* |_|_| |_|\__,_|\___|_| |_|\__| *
//*                                *
//===- tools/genromfs/indent.hpp ------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#ifndef PSTORE_GENROMFS_INDENT_HPP
#define PSTORE_GENROMFS_INDENT_HPP

#define PSTORE_GENROMFS_INDENT "    "
constexpr char indent[] = PSTORE_GENROMFS_INDENT;
constexpr char crindent[] = "\n" PSTORE_GENROMFS_INDENT;
#undef PSTORE_GENROMFS_INDENT

#endif // PSTORE_GENROMFS_INDENT_HPP
