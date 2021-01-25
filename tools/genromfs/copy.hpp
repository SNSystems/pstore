//*                         *
//*   ___ ___  _ __  _   _  *
//*  / __/ _ \| '_ \| | | | *
//* | (_| (_) | |_) | |_| | *
//*  \___\___/| .__/ \__, | *
//*           |_|    |___/  *
//===- tools/genromfs/copy.hpp --------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_GENROMFS_COPY_HPP
#define PSTORE_GENROMFS_COPY_HPP

#include <string>

void copy (std::string const & path, unsigned file_no);

#endif // PSTORE_GENROMFS_COPY_HPP
