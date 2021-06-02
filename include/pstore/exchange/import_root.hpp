//===- include/pstore/exchange/import_root.hpp ------------*- mode: C++ -*-===//
//*  _                            _                     _    *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __ ___   ___ | |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '__/ _ \ / _ \| __| *
//* | | | | | | | |_) | (_) | |  | |_  | | | (_) | (_) | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_|  \___/ \___/ \__| *
//*             |_|                                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file import_root.hpp
/// \brief Defines the root class which forms the initial rule for parsing pstore exchange files.

#ifndef PSTORE_EXCHANGE_IMPORT_ROOT_HPP
#define PSTORE_EXCHANGE_IMPORT_ROOT_HPP

#include "pstore/exchange/import_rule.hpp"
#include "pstore/json/json.hpp"

namespace pstore {

    class database;

    namespace exchange {
        namespace import_ns {

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

            /// Creates a JSON parser instance which will consume pstore exchange input.
            /// \param db  The database into which the imported data will be written.
            /// \returns A JSON parser instance.
            json::parser<callbacks> create_parser (database & db);

        } // end namespace import_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_ROOT_HPP
