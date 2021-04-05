//===- include/pstore/exchange/import_names.hpp -----------*- mode: C++ -*-===//
//*  _                            _                                      *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __   __ _ _ __ ___   ___  ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '_ \ / _` | '_ ` _ \ / _ \/ __| *
//* | | | | | | | |_) | (_) | |  | |_  | | | | (_| | | | | | |  __/\__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_| |_|\__,_|_| |_| |_|\___||___/ *
//*             |_|                                                      *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file import_names.hpp
/// \brief Contains the class which maps from string indexes to their store address.
///
/// Names are exported as an array of their own. We then refer to strings by their index in
/// that array. This has the advantage that we know that each string will appear only once
/// in the exported output.
///
/// The class in this module is responsible for gathering the strings in that exported array and
/// then for converting a reference index to the string's indirect address in the store.

#ifndef PSTORE_EXCHANGE_IMPORT_NAMES_HPP
#define PSTORE_EXCHANGE_IMPORT_NAMES_HPP

#include <unordered_map>

#include "pstore/core/hamt_set.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/exchange/import_error.hpp"
#include "pstore/exchange/import_rule.hpp"

namespace pstore {
    namespace exchange {
        namespace import_ns {

            class name_mapping {
            public:
                name_mapping () = default;
                name_mapping (name_mapping const &) = delete;
                name_mapping (name_mapping &&) noexcept = delete;

                ~name_mapping () noexcept = default;

                name_mapping & operator= (name_mapping const &) = delete;
                name_mapping & operator= (name_mapping &&) noexcept = delete;

                std::error_code add_string (not_null<transaction_base *> transaction,
                                            std::string const & str);

                void flush (not_null<transaction_base *> transaction);

                error_or<typed_address<indirect_string>> lookup (std::uint64_t index) const;
                std::size_t size () const noexcept {
                    PSTORE_ASSERT (strings_.size () == views_.size ());
                    return strings_.size ();
                }

            private:
                indirect_string_adder adder_;
                std::list<std::string> strings_;
                std::list<raw_sstring_view> views_;

                std::unordered_map<std::uint64_t, typed_address<indirect_string>> lookup_;
            };

        } // end namespace import_ns
    }     // end namespace exchange
} // namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_NAMES_HPP
