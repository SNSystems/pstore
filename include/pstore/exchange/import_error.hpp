//*  _                            _                               *
//* (_)_ __ ___  _ __   ___  _ __| |_    ___ _ __ _ __ ___  _ __  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __|  / _ \ '__| '__/ _ \| '__| *
//* | | | | | | | |_) | (_) | |  | |_  |  __/ |  | | | (_) | |    *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \___|_|  |_|  \___/|_|    *
//*             |_|                                               *
//===- include/pstore/exchange/import_error.hpp ---------------------------===//
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
#ifndef PSTORE_EXCHANGE_IMPORT_ERROR_HPP
#define PSTORE_EXCHANGE_IMPORT_ERROR_HPP

#include <system_error>

namespace pstore {
    namespace exchange {

        enum class import_error : int {
            none,

            alignment_must_be_power_of_2,
            alignment_is_too_great,
            duplicate_name,
            no_such_name,
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
            debug_line_header_digest_not_found,
        };


        class import_error_category : public std::error_category {
        public:
            import_error_category () noexcept;
            char const * name () const noexcept override;
            std::string message (int error) const override;
        };

        std::error_category const & get_import_error_category () noexcept;
        std::error_code make_error_code (import_error e) noexcept;

    } // end namespace exchange
} // end namespace pstore

namespace std {

    template <>
    struct is_error_code_enum<pstore::exchange::import_error> : std::true_type {};

} // end namespace std

#endif // PSTORE_EXCHANGE_IMPORT_ERROR_HPP
