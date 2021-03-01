//===- include/pstore/exchange/import_error.hpp -----------*- mode: C++ -*-===//
//*  _                            _                               *
//* (_)_ __ ___  _ __   ___  _ __| |_    ___ _ __ _ __ ___  _ __  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __|  / _ \ '__| '__/ _ \| '__| *
//* | | | | | | | |_) | (_) | |  | |_  |  __/ |  | | | (_) | |    *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \___|_|  |_|  \___/|_|    *
//*             |_|                                               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file import_error.hpp
/// \brief  Declares the errors that can be issued by the exchange import library.
#ifndef PSTORE_EXCHANGE_IMPORT_ERROR_HPP
#define PSTORE_EXCHANGE_IMPORT_ERROR_HPP

#include <system_error>

namespace pstore {
    namespace exchange {
        namespace import {

            enum class error : int {
                none,

                alignment_must_be_power_of_2,
                alignment_is_too_great,
                duplicate_name,
                no_such_name,
                no_such_compilation,
                no_such_fragment,

                unexpected_null,
                unexpected_boolean,
                unexpected_number,
                unexpected_string,
                unexpected_array,
                unexpected_end_array,
                unexpected_object,
                unexpected_object_key,
                unexpected_end_object,

                bss_section_was_incomplete,
                definition_was_incomplete,
                unrecognized_ifixup_key,
                ifixup_object_was_incomplete,
                unrecognized_xfixup_key,
                xfixup_object_was_incomplete,
                generic_section_was_incomplete,
                unrecognized_section_object_key,
                unrecognized_root_key,
                root_object_was_incomplete,
                unknown_transaction_object_key,
                unknown_compilation_object_key,
                unknown_definition_object_key,

                incomplete_compilation_object,
                incomplete_debug_line_section,
                incomplete_linked_definition_object,

                bad_digest,
                bad_base64_data,
                bad_linkage,
                bad_uuid,
                bad_visibility,
                unknown_section_name,
                index_out_of_range,
                debug_line_header_digest_not_found,
                number_too_large,
            };


            class error_category final : public std::error_category {
            public:
                error_category () noexcept;
                char const * name () const noexcept override;
                std::string message (int error) const override;
            };

            std::error_category const & get_import_error_category () noexcept;
            std::error_code make_error_code (error e) noexcept;

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

namespace std {

    template <>
    struct is_error_code_enum<pstore::exchange::import::error> : std::true_type {};

} // end namespace std

#endif // PSTORE_EXCHANGE_IMPORT_ERROR_HPP
