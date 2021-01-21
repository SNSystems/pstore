//*  _                            _                     _    *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __ ___   ___ | |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '__/ _ \ / _ \| __| *
//* | | | | | | | |_) | (_) | |  | |_  | | | (_) | (_) | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_|  \___/ \___/ \__| *
//*             |_|                                          *
//===- include/pstore/exchange/import_root.hpp ----------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
