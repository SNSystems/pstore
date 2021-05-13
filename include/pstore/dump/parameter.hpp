//===- include/pstore/dump/parameter.hpp ------------------*- mode: C++ -*-===//
//*                                       _             *
//*  _ __   __ _ _ __ __ _ _ __ ___   ___| |_ ___ _ __  *
//* | '_ \ / _` | '__/ _` | '_ ` _ \ / _ \ __/ _ \ '__| *
//* | |_) | (_| | | | (_| | | | | | |  __/ ||  __/ |    *
//* | .__/ \__,_|_|  \__,_|_| |_| |_|\___|\__\___|_|    *
//* |_|                                                 *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file parameter.hpp
/// \brief Declares a struct which carries the display parameters for the dump output.

#ifndef PSTORE_DUMP_PARAMETER_HPP
#define PSTORE_DUMP_PARAMETER_HPP

#include <string>

#include "pstore/config/config.hpp"

#ifdef PSTORE_IS_INSIDE_LLVM
#    include "llvm/ADT/Triple.h"
#endif

namespace pstore {
    class database;

    namespace dump {

        struct parameters {
            parameters (database const & db_, bool const hex_mode_, bool const expanded_addresses_,
                        bool const no_times_, bool const no_disassembly_,
                        std::string const & triple_)
                    : db{db_}
                    , hex_mode{hex_mode_}
                    , expanded_addresses{expanded_addresses_}
                    , no_times{no_times_}
#ifdef PSTORE_IS_INSIDE_LLVM
                    , no_disassembly{no_disassembly_}
                    , triple{triple_}
#endif
                    {
                        (void) triple_;
                        (void) no_disassembly_;
                    }
            parameters (parameters const &) = delete;
            parameters (parameters &&) noexcept = delete;

            parameters & operator= (parameters const &) = delete;
            parameters & operator= (parameters &&) noexcept = delete;

            database const & db;
            bool const hex_mode;
            bool const expanded_addresses;
            bool const no_times;
#ifdef PSTORE_IS_INSIDE_LLVM
            bool const no_disassembly;
            llvm::Triple const triple;
#endif
        };

    } // end namespace dump
} // end namespace pstore

#endif // PSTORE_DUMP_PARAMETER_HPP
