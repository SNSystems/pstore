//*  _                            _                               *
//* (_)_ __ ___  _ __   ___  _ __| |_    ___ _ __ _ __ ___  _ __  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __|  / _ \ '__| '__/ _ \| '__| *
//* | | | | | | | |_) | (_) | |  | |_  |  __/ |  | | | (_) | |    *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \___|_|  |_|  \___/|_|    *
//*             |_|                                               *
//===- lib/exchange/import_error.cpp --------------------------------------===//
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
#include "pstore/exchange/import_error.hpp"

#include <string>

namespace pstore {
    namespace exchange {

        import_error_category::import_error_category () noexcept = default;

        char const * import_error_category::name () const noexcept {
            return "import parser category";
        }

        std::string import_error_category::message (int const error) const {
            auto const * result = "unknown import_error_category error";
            switch (static_cast<import_error> (error)) {
            case import_error::none: result = "none"; break;

            case import_error::alignment_must_be_power_of_2:
                result = "alignment must be a power of 2";
                break;
            case import_error::alignment_is_too_great:
                result = "alignment value is too large to be represented";
                break;
            case import_error::duplicate_name: result = "duplicate name"; break;
            case import_error::no_such_name: result = "no such name"; break;
            case import_error::no_such_fragment: result = "no such fragment"; break;
            case import_error::unexpected_null: result = "unexpected null"; break;
            case import_error::unexpected_boolean: result = "unexpected boolean"; break;
            case import_error::unexpected_number: result = "unexpected number"; break;
            case import_error::unexpected_string: result = "unexpected string"; break;
            case import_error::unexpected_array: result = "unexpected array"; break;
            case import_error::unexpected_end_array: result = "unexpected end of array"; break;
            case import_error::unexpected_object: result = "unexpected object"; break;
            case import_error::unexpected_object_key: result = "unexpected object key"; break;
            case import_error::unexpected_end_object: result = "unexpected end object"; break;


            case import_error::bss_section_was_incomplete:
                result = "bss section was incomplete";
                break;
            case import_error::definition_was_incomplete:
                result = "definition was incomplete";
                break;
            case import_error::unrecognized_ifixup_key:
                result = "unrecognized internal fixup object key";
                break;
            case import_error::ifixup_object_was_incomplete:
                result = "internal fixup object was not complete";
                break;
            case import_error::unrecognized_xfixup_key:
                result = "unrecognized external fixup object key";
                break;
            case import_error::xfixup_object_was_incomplete:
                result = "external fixup object was incomplete";
                break;
            case import_error::unrecognized_section_object_key:
                result = "unrecognized section object key";
                break;
            case import_error::generic_section_was_incomplete:
                result = "generic section object was incomplete";
                break;
            case import_error::root_object_was_incomplete:
                result = "root object was incomplete";
                break;
            case import_error::unrecognized_root_key:
                result = "unrecognized root object key";
                break;
            case import_error::unknown_transaction_object_key:
                result = "unrecognized transaction object key";
                break;
            case import_error::unknown_compilation_object_key:
                result = "unknown compilation object key";
                break;
            case import_error::unknown_definition_object_key:
                result = "unknown definition object key";
                break;

            case import_error::incomplete_debug_line_section:
                result = "debug line section object was incomplete";
                break;
            case import_error::incomplete_compilation_object:
                result = "compilation object was incomplete";
                break;
            case import_error::incomplete_linked_definition_object:
                result = "linked-definition object was incomplete";
                break;

            case import_error::bad_linkage: result = "unknown linkage type"; break;
            case import_error::bad_uuid: result = "bad UUID"; break;
            case import_error::bad_base64_data: result = "bad Base64 data"; break;
            case import_error::bad_digest: result = "bad digest"; break;
            case import_error::bad_visibility: result = "unknown visibility"; break;
            case import_error::unknown_section_name: result = "unknown section name"; break;

            case import_error::debug_line_header_digest_not_found:
                result = "debug line header digest was not found";
                break;
            }
            return result;
        }

        std::error_category const & get_import_error_category () noexcept {
            static import_error_category const cat;
            return cat;
        }

        std::error_code make_error_code (import_error const e) noexcept {
            static_assert (std::is_same<std::underlying_type<import_error>::type, int>::value,
                           "The underlying type of import error must be int");
            return {static_cast<int> (e), get_import_error_category ()};
        }

    } // end namespace exchange
} // end namespace pstore
