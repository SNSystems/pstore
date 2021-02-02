//*                                          _       _    __  *
//* __   ____ _  ___ _   _ _   _ _ __ ___   (_)_ __ | |_ / _| *
//* \ \ / / _` |/ __| | | | | | | '_ ` _ \  | | '_ \| __| |_  *
//*  \ V / (_| | (__| |_| | |_| | | | | | | | | | | | |_|  _| *
//*   \_/ \__,_|\___|\__,_|\__,_|_| |_| |_| |_|_| |_|\__|_|   *
//*                                                           *
//===- lib/core/vacuum_intf.cpp -------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/core/vacuum_intf.hpp"

#include "pstore/core/time.hpp"

namespace pstore {
    //*     _                    _  *
    //*  __| |_  __ _ _ _ ___ __| | *
    //* (_-< ' \/ _` | '_/ -_) _` | *
    //* /__/_||_\__,_|_| \___\__,_| *
    //*                             *
    shared::shared ()
            : open_tick (milliseconds_since_epoch ()) {}

} // namespace pstore
