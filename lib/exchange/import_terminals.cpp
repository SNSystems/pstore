//*  _                            _     _                      _             _      *
//* (_)_ __ ___  _ __   ___  _ __| |_  | |_ ___ _ __ _ __ ___ (_)_ __   __ _| |___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | __/ _ \ '__| '_ ` _ \| | '_ \ / _` | / __| *
//* | | | | | | | |_) | (_) | |  | |_  | ||  __/ |  | | | | | | | | | | (_| | \__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \__\___|_|  |_| |_| |_|_|_| |_|\__,_|_|___/ *
//*             |_|                                                                 *
//===- lib/exchange/import_terminals.cpp ----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_terminals.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            //*       _     _    __ _ _             _      *
            //*  _  _(_)_ _| |_ / /| | |   _ _ _  _| |___  *
            //* | || | | ' \  _/ _ \_  _| | '_| || | / -_) *
            //*  \_,_|_|_||_\__\___/ |_|  |_|  \_,_|_\___| *
            //*                                            *
            // uint64 value
            // ~~~~~~~~~~~~
            std::error_code uint64_rule::uint64_value (std::uint64_t const v) {
                *v_ = v;
                return pop ();
            }

            gsl::czstring uint64_rule::name () const noexcept { return "uint64 rule"; }

            //*     _       _                      _      *
            //*  __| |_ _ _(_)_ _  __ _   _ _ _  _| |___  *
            //* (_-<  _| '_| | ' \/ _` | | '_| || | / -_) *
            //* /__/\__|_| |_|_||_\__, | |_|  \_,_|_\___| *
            //*                   |___/                   *
            // name
            // ~~~~
            gsl::czstring string_rule::name () const noexcept { return "string rule"; }

            // string value
            // ~~~~~~~~~~~~
            std::error_code string_rule::string_value (std::string const & v) {
                *v_ = v;
                return pop ();
            }

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore
