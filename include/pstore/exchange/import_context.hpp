//===- include/pstore/exchange/import_context.hpp ---------*- mode: C++ -*-===//
//*  _                            _                     _            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_    ___ ___  _ __ | |_ _____  _| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __|  / __/ _ \| '_ \| __/ _ \ \/ / __| *
//* | | | | | | | |_) | (_) | |  | |_  | (_| (_) | | | | ||  __/>  <| |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \___\___/|_| |_|\__\___/_/\_\\__| *
//*             |_|                                                       *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_IMPORT_CONTEXT_HPP
#define PSTORE_EXCHANGE_IMPORT_CONTEXT_HPP

#include <list>
#include <stack>

#include "pstore/support/gsl.hpp"

namespace pstore {
    class database;
    class transaction_base;

    namespace exchange {
        namespace import_ns {

            class patcher {
            public:
                virtual ~patcher () noexcept = default;
                virtual std::error_code operator() (transaction_base * t) = 0;
            };

            class rule;

            struct context {
                explicit context (gsl::not_null<database *> const db_) noexcept
                        : db{db_} {}
                context (context const &) = delete;
                context (context &&) = delete;

                ~context () noexcept = default;

                context & operator= (context const &) = delete;
                context & operator= (context &&) = delete;

                std::error_code apply_patches (transaction_base * const t) {
                    for (auto const & patch : patches) {
                        if (std::error_code const erc = (*patch) (t)) {
                            return erc;
                        }
                    }
                    // Ensure that we can't apply the patches more than once.
                    patches.clear ();
                    return {};
                }

                gsl::not_null<database *> const db;
                std::stack<std::unique_ptr<rule>> stack;
                std::list<std::unique_ptr<patcher>> patches;
            };

        } // end namespace import_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_CONTEXT_HPP
