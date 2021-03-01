//===- include/pstore/support/head_revision.hpp -----------*- mode: C++ -*-===//
//*  _                    _                  _     _              *
//* | |__   ___  __ _  __| |  _ __ _____   _(_)___(_) ___  _ __   *
//* | '_ \ / _ \/ _` |/ _` | | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* | | | |  __/ (_| | (_| | | | |  __/\ V /| \__ \ | (_) | | | | *
//* |_| |_|\___|\__,_|\__,_| |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                                               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_SUPPORT_HEAD_REVISION_HPP
#define PSTORE_SUPPORT_HEAD_REVISION_HPP

#include <limits>

namespace pstore {

    using revision_number = unsigned;
    constexpr revision_number head_revision = std::numeric_limits<revision_number>::max ();

} // namespace pstore

#endif // PSTORE_SUPPORT_HEAD_REVISION_HPP
