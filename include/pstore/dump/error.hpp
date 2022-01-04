//===- include/pstore/dump/error.hpp ----------------------*- mode: C++ -*-===//
//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_DUMP_ERROR_HPP
#define PSTORE_DUMP_ERROR_HPP

#include <string>
#include <system_error>

namespace pstore {
    namespace dump {

        enum class error_code : int {
            cant_find_target = 1,
            no_register_info_for_target,
            no_assembly_info_for_target,
            no_subtarget_info_for_target,
            no_instruction_info_for_target,
            no_disassembler_for_target,
            no_instruction_printer,
            cannot_create_asm_streamer,
            unknown_target,
        };

        class error_category final : public std::error_category {
        public:
            // The need for this constructor was removed by CWG defect 253 but Clang (prior
            // to 3.9.0) and GCC (before 4.6.4) require its presence.
            error_category () noexcept {}
            char const * name () const noexcept override;
            std::string message (int error) const override;
        };

        std::error_code make_error_code (error_code e);

    } // end namespace dump
} // end namespace pstore

namespace std {

    template <>
    struct is_error_code_enum<pstore::dump::error_code> : std::true_type {};

} // end namespace std

#endif // PSTORE_DUMP_ERROR_HPP
