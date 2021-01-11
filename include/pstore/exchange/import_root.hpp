//*  _                            _                     _    *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __ ___   ___ | |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '__/ _ \ / _ \| __| *
//* | | | | | | | |_) | (_) | |  | |_  | | | (_) | (_) | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_|  \___/ \___/ \__| *
//*             |_|                                          *
//===- include/pstore/exchange/import_root.hpp ----------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_EXCHANGE_IMPORT_ROOT_HPP
#define PSTORE_EXCHANGE_IMPORT_ROOT_HPP

#include "pstore/exchange/import_rule.hpp"
#include "pstore/json/json.hpp"

namespace pstore {

    class database;

    namespace exchange {
        namespace import {

            class root final : public rule {
            public:
                explicit root (not_null<context *> const ctxt) noexcept
                        : rule (ctxt) {}
                root (root const &) = delete;
                root (root &&) noexcept = delete;
                ~root () noexcept override = default;

                root & operator= (root const &) = delete;
                root & operator= (root &&) noexcept = delete;

                gsl::czstring name () const noexcept override;
                std::error_code begin_object () override;

            private:
                std::error_code apply_patches ();
            };


            json::parser<callbacks> create_parser (database & db);

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_ROOT_HPP
