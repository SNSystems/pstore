//===- include/pstore/vacuum/copy.hpp ---------------------*- mode: C++ -*-===//
//*                         *
//*   ___ ___  _ __  _   _  *
//*  / __/ _ \| '_ \| | | | *
//* | (_| (_) | |_) | |_| | *
//*  \___\___/| .__/ \__, | *
//*           |_|    |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Copyright (c) 2015-2017 by Sony Interactive Entertainment, Inc.
/// \file copy.hpp

#ifndef VACUUM_COPY_HPP
#define VACUUM_COPY_HPP (1)

#include <memory>

namespace pstore {
    class database;
}
namespace vacuum {
    struct status;
    struct user_options;
    void copy (std::shared_ptr<pstore::database> source, status * const st,
               user_options const & opt);
} // namespace vacuum

#endif // VACUUM_COPY_HPP
