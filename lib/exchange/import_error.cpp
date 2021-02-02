//*  _                            _                               *
//* (_)_ __ ___  _ __   ___  _ __| |_    ___ _ __ _ __ ___  _ __  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __|  / _ \ '__| '__/ _ \| '__| *
//* | | | | | | | |_) | (_) | |  | |_  |  __/ |  | | | (_) | |    *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \___|_|  |_|  \___/|_|    *
//*             |_|                                               *
//===- lib/exchange/import_error.cpp --------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_error.hpp"

#include <string>

namespace {

    std::error_category const & get_error_category () noexcept {
        static pstore::exchange::import::error_category const cat;
        return cat;
    }

} // end anonymous namespace

namespace pstore {
    namespace exchange {
        namespace import {

            error_category::error_category () noexcept = default;

            char const * error_category::name () const noexcept { return "pstore import category"; }

            std::string error_category::message (int const err) const {
                auto const * result = "unknown import error_category error";
                switch (static_cast<error> (err)) {
                case error::none: result = "none"; break;

                case error::alignment_must_be_power_of_2:
                    result = "alignment must be a power of 2";
                    break;
                case error::alignment_is_too_great:
                    result = "alignment value is too large to be represented";
                    break;
                case error::duplicate_name: result = "duplicate name"; break;
                case error::no_such_name: result = "no such name"; break;
                case error::no_such_compilation: result = "no such compilation"; break;
                case error::no_such_fragment: result = "no such fragment"; break;
                case error::unexpected_null: result = "unexpected null"; break;
                case error::unexpected_boolean: result = "unexpected boolean"; break;
                case error::unexpected_number: result = "unexpected number"; break;
                case error::unexpected_string: result = "unexpected string"; break;
                case error::unexpected_array: result = "unexpected array"; break;
                case error::unexpected_end_array: result = "unexpected end of array"; break;
                case error::unexpected_object: result = "unexpected object"; break;
                case error::unexpected_object_key: result = "unexpected object key"; break;
                case error::unexpected_end_object: result = "unexpected end object"; break;


                case error::bss_section_was_incomplete:
                    result = "bss section was incomplete";
                    break;
                case error::definition_was_incomplete: result = "definition was incomplete"; break;
                case error::unrecognized_ifixup_key:
                    result = "unrecognized internal fixup object key";
                    break;
                case error::ifixup_object_was_incomplete:
                    result = "internal fixup object was not complete";
                    break;
                case error::unrecognized_xfixup_key:
                    result = "unrecognized external fixup object key";
                    break;
                case error::xfixup_object_was_incomplete:
                    result = "external fixup object was incomplete";
                    break;
                case error::unrecognized_section_object_key:
                    result = "unrecognized section object key";
                    break;
                case error::generic_section_was_incomplete:
                    result = "generic section object was incomplete";
                    break;
                case error::root_object_was_incomplete:
                    result = "root object was incomplete";
                    break;
                case error::unrecognized_root_key: result = "unrecognized root object key"; break;
                case error::unknown_transaction_object_key:
                    result = "unrecognized transaction object key";
                    break;
                case error::unknown_compilation_object_key:
                    result = "unknown compilation object key";
                    break;
                case error::unknown_definition_object_key:
                    result = "unknown definition object key";
                    break;

                case error::incomplete_debug_line_section:
                    result = "debug line section object was incomplete";
                    break;
                case error::incomplete_compilation_object:
                    result = "compilation object was incomplete";
                    break;
                case error::incomplete_linked_definition_object:
                    result = "linked-definition object was incomplete";
                    break;

                case error::bad_linkage: result = "unknown linkage type"; break;
                case error::bad_uuid: result = "bad UUID"; break;
                case error::bad_base64_data: result = "bad Base64 data"; break;
                case error::bad_digest: result = "bad digest"; break;
                case error::bad_visibility: result = "unknown visibility"; break;
                case error::index_out_of_range: result = "index out of range"; break;
                case error::unknown_section_name: result = "unknown section name"; break;

                case error::debug_line_header_digest_not_found:
                    result = "debug line header digest was not found";
                    break;
                }
                return result;
            }

            std::error_code make_error_code (error const e) noexcept {
                static_assert (std::is_same<std::underlying_type<error>::type, int>::value,
                               "The underlying type of import error must be int");
                return {static_cast<int> (e), get_error_category ()};
            }

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore
