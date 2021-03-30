//===- include/pstore/exchange/export_paths.hpp -----------*- mode: C++ -*-===//
//*                             _                 _   _          *
//*   _____  ___ __   ___  _ __| |_   _ __   __ _| |_| |__  ___  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| | '_ \ / _` | __| '_ \/ __| *
//* |  __/>  <| |_) | (_) | |  | |_  | |_) | (_| | |_| | | \__ \ *
//*  \___/_/\_\ .__/ \___/|_|   \__| | .__/ \__,_|\__|_| |_|___/ *
//*           |_|                    |_|                         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file export_paths.hpp
/// \brief  Exporting the members of the paths index.

#ifndef PSTORE_EXCHANGE_EXPORT_PATHS_HPP
#define PSTORE_EXCHANGE_EXPORT_PATHS_HPP

#include "pstore/exchange/export_names.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            void emit_paths (ostream_base & os, indent const ind, database const & db,
                             unsigned const generation,
                             gsl::not_null<name_mapping *> const string_table);

        } // end namespace export_ns
    }     // end namespace exchange
} // namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_PATHS_HPP
