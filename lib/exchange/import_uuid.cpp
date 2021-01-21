//*  _                            _                 _     _  *
//* (_)_ __ ___  _ __   ___  _ __| |_   _   _ _   _(_) __| | *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | | | | | | | |/ _` | *
//* | | | | | | | |_) | (_) | |  | |_  | |_| | |_| | | (_| | *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \__,_|\__,_|_|\__,_| *
//*             |_|                                          *
//===- lib/exchange/import_uuid.cpp ---------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_uuid.hpp"
#include "pstore/exchange/import_error.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            uuid_rule::uuid_rule (not_null<context *> const ctxt, not_null<uuid *> const v) noexcept
                    : rule (ctxt)
                    , v_{v} {}

            std::error_code uuid_rule::string_value (std::string const & v) {
                if (maybe<uuid> const value = uuid::from_string (v)) {
                    *v_ = *value;
                    return pop ();
                }
                return error::bad_uuid;
            }

            gsl::czstring uuid_rule::name () const noexcept { return "uuid"; }

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore
