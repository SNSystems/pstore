//*  _                            _         _      _                  *
//* (_)_ __ ___  _ __   ___  _ __| |_    __| | ___| |__  _   _  __ _  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __|  / _` |/ _ \ '_ \| | | |/ _` | *
//* | | | | | | | |_) | (_) | |  | |_  | (_| |  __/ |_) | |_| | (_| | *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \__,_|\___|_.__/ \__,_|\__, | *
//*             |_|                                            |___/  *
//*  _ _              _                    _            *
//* | (_)_ __   ___  | |__   ___  __ _  __| | ___ _ __  *
//* | | | '_ \ / _ \ | '_ \ / _ \/ _` |/ _` |/ _ \ '__| *
//* | | | | | |  __/ | | | |  __/ (_| | (_| |  __/ |    *
//* |_|_|_| |_|\___| |_| |_|\___|\__,_|\__,_|\___|_|    *
//*                                                     *
//===- include/pstore/exchange/import_debug_line_header.hpp ---------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_IMPORT_DEBUG_LINE_HEADER_HPP
#define PSTORE_EXCHANGE_IMPORT_DEBUG_LINE_HEADER_HPP

#include "pstore/core/hamt_map.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/exchange/import_error.hpp"
#include "pstore/exchange/import_rule.hpp"
#include "pstore/support/base64.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            class debug_line_index final : public rule {
            public:
                debug_line_index (not_null<context *> ctxt,
                                  not_null<transaction_base *> transaction);
                debug_line_index (debug_line_index const &) = delete;
                debug_line_index (debug_line_index &&) noexcept = delete;

                ~debug_line_index () noexcept override = default;

                debug_line_index & operator= (debug_line_index const &) = delete;
                debug_line_index & operator= (debug_line_index &&) noexcept = delete;

                gsl::czstring name () const noexcept override;

                std::error_code string_value (std::string const & s) override;
                std::error_code key (std::string const & s) override;
                std::error_code end_object () override;

            private:
                std::shared_ptr<index::debug_line_header_index> index_;
                index::digest digest_;

                not_null<transaction_base *> transaction_;
            };

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_DEBUG_LINE_HEADER_HPP
