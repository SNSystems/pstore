//===- include/pstore/exchange/export_compilation.hpp -----*- mode: C++ -*-===//
//*                             _    *
//*   _____  ___ __   ___  _ __| |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| *
//* |  __/>  <| |_) | (_) | |  | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__| *
//*           |_|                    *
//*                            _ _       _   _              *
//*   ___ ___  _ __ ___  _ __ (_) | __ _| |_(_) ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ \| | |/ _` | __| |/ _ \| '_ \  *
//* | (_| (_) | | | | | | |_) | | | (_| | |_| | (_) | | | | *
//*  \___\___/|_| |_| |_| .__/|_|_|\__,_|\__|_|\___/|_| |_| *
//*                     |_|                                 *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file export_compilation.hpp
/// \brief  Functions for exporting compilations and the compilation index.
#ifndef PSTORE_EXCHANGE_EXPORT_COMPILATION_HPP
#define PSTORE_EXCHANGE_EXPORT_COMPILATION_HPP

#include "pstore/exchange/export_strings.hpp"
#include "pstore/mcrepo/compilation.hpp"

namespace pstore {
    namespace exchange {
        namespace export_ns {

            class ostream_base;

            void emit_compilation (ostream_base & os, indent ind, database const & db,
                                   repo::compilation const & compilation,
                                   string_mapping const & strings, bool comments);

            void emit_compilation_index (ostream_base & os, indent ind, database const & db,
                                         unsigned generation, string_mapping const & strings,
                                         bool comments);

        } // end namespace export_ns
    }     // end namespace exchange

    namespace repo {

        exchange::export_ns::ostream_base & operator<< (exchange::export_ns::ostream_base & os,
                                                        linkage l);

        exchange::export_ns::ostream_base & operator<< (exchange::export_ns::ostream_base & os,
                                                        visibility v);

    } // end namespace repo
} // end namespace pstore

#endif // PSTORE_EXCHANGE_EXPORT_COMPILATION_HPP
