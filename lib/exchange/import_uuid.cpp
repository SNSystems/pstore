//*  _                            _                 _     _  *
//* (_)_ __ ___  _ __   ___  _ __| |_   _   _ _   _(_) __| | *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | | | | | | | |/ _` | *
//* | | | | | | | |_) | (_) | |  | |_  | |_| | |_| | | (_| | *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \__,_|\__,_|_|\__,_| *
//*             |_|                                          *
//===- lib/exchange/import_uuid.cpp ---------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
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
    } // end namespace exchange
} // end namespace pstore
