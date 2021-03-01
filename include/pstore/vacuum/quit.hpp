//===- include/pstore/vacuum/quit.hpp ---------------------*- mode: C++ -*-===//
//*              _ _    *
//*   __ _ _   _(_) |_  *
//*  / _` | | | | | __| *
//* | (_| | |_| | | |_  *
//*  \__, |\__,_|_|\__| *
//*     |_|             *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/// \file vacuum/quit.hpp

#ifndef VACUUM_QUIT_HPP
#define VACUUM_QUIT_HPP (1)

#include <cstdlib>
#include <memory>
#include <thread>

namespace pstore {
    class database;
}
namespace vacuum {
    struct status;
}

void notify_quit_thread ();
std::thread create_quit_thread (vacuum::status & status,
                                std::shared_ptr<pstore::database> const & src_db);

#endif // VACUUM_QUIT_HPP
