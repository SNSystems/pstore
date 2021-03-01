//===- lib/romfs/dirent.cpp -----------------------------------------------===//
//*      _ _                _    *
//*   __| (_)_ __ ___ _ __ | |_  *
//*  / _` | | '__/ _ \ '_ \| __| *
//* | (_| | | | |  __/ | | | |_  *
//*  \__,_|_|_|  \___|_| |_|\__| *
//*                              *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/romfs/dirent.hpp"

#include <stdexcept>

#include "pstore/romfs/romfs.hpp"

auto pstore::romfs::dirent::opendir () const -> error_or<class directory const * PSTORE_NONNULL> {
    if (!is_directory () || stat_.size != sizeof (directory const *)) {
        return error_or<directory const *> (make_error_code (error_code::enotdir));
    }
    return error_or<directory const *> (reinterpret_cast<directory const *> (contents_));
}
