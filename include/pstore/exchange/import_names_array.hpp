//===- include/pstore/exchange/import_names_array.hpp -----*- mode: C++ -*-===//
//*  _                            _                                      *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __   __ _ _ __ ___   ___  ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '_ \ / _` | '_ ` _ \ / _ \/ __| *
//* | | | | | | | |_) | (_) | |  | |_  | | | | (_| | | | | | |  __/\__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_| |_|\__,_|_| |_| |_|\___||___/ *
//*             |_|                                                      *
//*                              *
//*   __ _ _ __ _ __ __ _ _   _  *
//*  / _` | '__| '__/ _` | | | | *
//* | (_| | |  | | | (_| | |_| | *
//*  \__,_|_|  |_|  \__,_|\__, | *
//*                       |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_IMPORT_NAMES_ARRAY_HPP
#define PSTORE_EXCHANGE_IMPORT_NAMES_ARRAY_HPP

#include "pstore/exchange/import_names.hpp"

namespace pstore {
    namespace exchange {
        namespace import_ns {

            class names_array_members final : public rule {
            public:
                names_array_members (not_null<context *> ctxt,
                                     not_null<transaction_base *> transaction,
                                     not_null<name_mapping *> names);
                names_array_members (names_array_members const &) = delete;
                names_array_members (names_array_members &&) noexcept = delete;

                ~names_array_members () noexcept override = default;

                names_array_members & operator= (names_array_members const &) = delete;
                names_array_members & operator= (names_array_members &&) noexcept = delete;

            private:
                std::error_code string_value (std::string const & str) override;
                std::error_code end_array () override;
                gsl::czstring name () const noexcept override;

                not_null<transaction_base *> const transaction_;
                not_null<name_mapping *> const names_;
            };

        } // end namespace import_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_NAMES_ARRAY_HPP
